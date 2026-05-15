# Phase 45: Startup JSONL Auto-Import - Research

**Researched:** 2026-05-15
**Domain:** mdCAD startup CLI parsing, non-UI JSONL import orchestration, embedded-safe error handling
**Confidence:** HIGH

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| JSON-01 | Developer can pass an absolute JSONL path and mdCAD auto-imports it at startup using the large flat dump workflow. | Extend `mdcad_launch_config_t` with a validated absolute `--jsonl` path, then arm a startup controller that calls the existing flat `jsonl_import_job_*` path outside `ui_scene_hierarchy_draw()`. |
| JSON-03 | User keeps a usable embedded viewer and gets a clear error state when startup JSONL import fails. | Treat bad files as runtime import failures, not fatal launch-config failures; latch a non-modal app-level error/progress surface and never quit the embedded session on startup import failure. |
</phase_requirements>

## Summary

The existing flat JSONL path is already the correct implementation seam. `src/ui/ui_scene_hierarchy.h` proves the approved workflow: `jsonl_import_job_start(...)` → optional mesh mode → `jsonl_import_job_set_observer_contract(...)` → sync-or-async completion → status text. Phase 45 should reuse that exact job contract, not create a second importer.

The missing piece is ownership. Today the flat import job is started and ticked from `ui_scene_hierarchy_draw()`, so startup import would silently depend on Scene Hierarchy UI state. The safest Phase 45 shape is a small non-UI startup controller, owned by `src/app.c`, armed from CLI parse state, started on the first frame, ticked every frame outside the `state.ui_visible` gate, and rendered through a lightweight app-level progress/error banner. That keeps embedded launch stable and preserves the Phase 44 input/lifecycle boundary.

**Primary recommendation:** Parse `--jsonl <absolute-path>` in `app_launch_config.h`, store it in `state.launch`, and drive the existing flat `jsonl_import_job_t` from an app-owned startup controller that starts in `frame()` and reports failure through a persistent non-modal app overlay instead of Scene Hierarchy status text.

## Standard Stack

### Core
| Library / Module | Version | Purpose | Why Standard |
|------------------|---------|---------|--------------|
| `src/app_launch_config.h` | repo-local | CLI contract for embedded launch and future startup JSONL path | Already the single parse seam used by `sokol_main()`. |
| `src/app.c` | repo-local | Owns startup lifecycle, frame loop, and embedded-safe control flow | Startup import must live here to avoid UI-state coupling. |
| `src/jsonl_import_job.h` | repo-local | Existing large flat JSONL import workflow | Already supports async chunking, sync threshold, mesh mode, status text, and observer metadata capture. |
| `src/jsonl_loader.h` | repo-local | Flat JSONL file open/scan/error enums | Provides file-read errors used by the existing workflow. |

### Supporting
| Library / Module | Version | Purpose | When to Use |
|------------------|---------|---------|-------------|
| `src/ui/ui_scene_hierarchy.h` | repo-local | Canonical existing flat-import submit logic and status conventions | Mirror/refactor its flat import submit flow, but do not keep startup execution here. |
| `src/components/jsonl_observer_comp.h` | repo-local | Observer metadata contract | Keep startup imports stamped with source metadata while leaving live observe OFF by default. |
| `src/jsonl_observer_system.h` | repo-local | Existing linked refresh semantics | Reuse metadata shape only; Phase 45 must not turn on live refresh. |
| `samples/avalonia-host/MainWindow.axaml.cs` | repo-local | Current embedded launch caller | Reference only; JSONL launch arg wiring remains Phase 47 scope. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| App-owned startup controller | Starting/ticking the job inside `ui_scene_hierarchy_draw()` | Reject. That stalls startup import whenever Scene Hierarchy UI is hidden or not drawn. |
| Existing `jsonl_import_job_t` | New embedded-only importer | Reject. Duplicates import semantics and risks Phase 35/38/40 behavior drift. |
| Runtime import failure after viewer init | Fatal file-open check during CLI parse | Reject. Missing/bad files must preserve a usable embedded viewer per JSON-03. |

**Installation:** None — reuse the existing repo stack.

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── app.c                              # startup arm/tick/draw integration
├── app_launch_config.h                # --jsonl parse contract
├── startup_jsonl_import_controller.h  # new small header-only non-UI controller
├── jsonl_import_job.h                 # existing flat import engine
├── components/jsonl_observer_comp.h   # linked metadata contract
└── ui/ui_scene_hierarchy.h            # existing flat import UI submit path to mirror/refactor
```

### Pattern 1: Parse launch contract separately from file import success
**What:** `sokol_main()` should reject malformed CLI usage, but it should not reject a syntactically valid absolute path just because the file later fails to open or parse.
**When to use:** Always for `--jsonl`.
**Example:**
```c
// Source: src/app.c:2845-2854 + src/app_launch_config.h
mdcad_launch_config_t launch_config = {0};
char launch_error[256] = {0};
if (!mdcad_launch_config_parse(argc, argv, &launch_config, launch_error, sizeof(launch_error))) {
    fprintf(stderr, "Embedded startup failed: %s\n", launch_error);
    exit(2);
}
```

**Recommended CLI contract**
- `--jsonl <absolute-path>` is optional in embedded and non-embedded launches.
- Fatal parse errors: duplicate flag, missing value, non-absolute path, buffer overflow/truncation.
- Non-fatal runtime failures: file missing, unreadable, parse error, import job error.
- Keep existing `--embedded` / `--parent-hwnd` rules unchanged.
- Keep unknown flags ignored unless they conflict with known launch flags; current tests depend on tolerant parsing.

### Pattern 2: Arm in `init()`, start/tick in `frame()`
**What:** Store the requested startup import in controller state during `init()`, but actually call `jsonl_import_job_start(...)` from `frame()` after ImGui is live and before world/render updates.
**When to use:** Any startup import request.
**Best control flow:**
1. `sokol_main()` parses `--jsonl` into `state.launch`.
2. `init()` initializes a startup-import controller and arms it if `state.launch.startup_jsonl_path[0] != '\0'`.
3. `frame()` immediately after `simgui_new_frame(...)`:
   - start the job once if pending,
   - tick the job if running,
   - copy/latch success or error text,
   - mark Scene Hierarchy cache dirty on completion.
4. Continue into normal `ecs_world_progress()`, `ecs_scene_update()`, render, and embedded input flow.

**Why here:** the current flat job is UI-driven, but the actual scene mutation still happens before `ecs_world_progress()`/`ecs_scene_update()` in the same frame. Keeping the startup controller at that same lifecycle point preserves behavior without requiring Scene Hierarchy to be open.

### Pattern 3: Reuse flat import defaults, but keep observer live-refresh OFF
**What:** Startup import should use the current flat import defaults: scale `1.0f`, JSONL colours ON, white fallback colour, no CoM shift, zero rotation, mesh mode `0`, and observer metadata captured with `link_enabled=false`.
**When to use:** All Phase 45 startup imports.
**Example:**
```c
// Source: src/ui/ui_scene_hierarchy.h:2485-2500
bool started = jsonl_import_job_start(&job, path, 1.0f, true,
                                      vec4_make(1, 1, 1, 1),
                                      false, 0.0f, 0.0f, 0.0f);
jsonl_import_job_set_mesh_mode(&job, 0);
jsonl_import_job_set_observer_contract(&job, false, path);
```

**Important:** call `jsonl_import_job_set_observer_contract(..., false, path)` even in Phase 45. That preserves existing linked-refresh metadata shape without enabling observe-by-default.

### Pattern 4: Error state must be app-level, not Scene-Hierarchy-local
**What:** `ui_scene_hierarchy.h` only exposes `last_status` inside the hierarchy panel, and it is always drawn green. That is not a sufficient startup failure surface for embedded launch.
**When to use:** Pending import, import error, optional completion acknowledgement.
**Recommended UI shape:**
- Non-modal overlay or small app-level window drawn from `app.c`
- Visible outside the Scene Hierarchy panel state
- Progress while running, red latched error on failure, dismissible by user
- Never blocks the viewport or calls `exit()` / `sapp_quit()` on import failure

### Anti-Patterns to Avoid
- **Do not start/tick startup import only inside `ui_scene_hierarchy_draw()`:** that recreates the UI-state dependency Phase 45 is explicitly removing.
- **Do not treat file-open/parse failure as a launch-config failure:** JSON-03 requires the viewer to stay alive.
- **Do not enable observer live refresh in Phase 45:** `linked=false` is required; live refresh belongs to Phase 46.
- **Do not update the Avalonia sample to pass `--jsonl` here:** roadmap assigns that to Phase 47.
- **Do not invent a second “embedded importer”:** use `jsonl_import_job_t`.

## Canonical File References

| File | Why it matters |
|------|----------------|
| `src/app_launch_config.h` | Single launch-argument parser; Phase 45 must extend this without breaking embedded argument validation. |
| `src/app.c` | Owns `sokol_main()`, `init()`, and `frame()`; this is where startup request state, frame-ordering, and embedded-safe overlay logic belong. |
| `src/jsonl_import_job.h` | The actual large flat import engine, including sync threshold, progress reporting, root-anchor creation, and observer metadata capture. |
| `src/jsonl_loader.h` | Defines file-open/parse errors and optional quick-scan behavior. |
| `src/ui/ui_scene_hierarchy.h` | Current canonical flat import submit/tick/status path; Phase 45 should mirror or extract from here, not redesign behavior. |
| `src/components/jsonl_observer_comp.h` | Defines linked metadata and message history fields that Phase 46 will reuse. |
| `src/jsonl_observer_system.h` | Shows current linked refresh semantics and confirms they are separate from initial import. |
| `src/tests/embed_launch_config_test.c` | Existing parser test seam; extend it for `--jsonl` absolute-path behavior and error cases. |
| `src/tests/jsonl_flat_import_options_contract_test.c` | Existing flat import contract test seam; good place to keep default option assumptions explicit. |
| `src/tests/jsonl_flat_anchor_scoped_ingest_test.c` | Guards flat root/entry/geometry hierarchy semantics that startup import must preserve. |
| `samples/avalonia-host/MainWindow.axaml.cs` | Confirms Phase 45 should not yet wire the sample host JSONL argument; it currently only passes embed arguments. |

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Startup flat import execution | A new embedded-only parser/importer | `jsonl_import_job_start/tick/reset` | The existing job already owns progress, chunking, and root-anchor semantics. |
| Launch-path path detection | Host-specific string heuristics in app code | The same absolute-path helper pattern already used in `imgui_storage.h` | Windows drive-letter and UNC handling already exist in repo patterns. |
| Startup failure UX | Silent stderr-only logging or Scene Hierarchy-only status | App-level latched overlay/banner plus stderr | Embedded users may never notice a panel-local green status line. |
| Phase 46 behavior | Manual observe polling enablement in Phase 45 | `jsonl_import_job_set_observer_contract(..., false, path)` | Keeps metadata ready without changing current refresh defaults. |

**Key insight:** the hard part is not import logic; it is choosing an ownership seam that survives embedded viewer-first layout and hidden panels.

## Common Pitfalls

### Pitfall 1: Breaking existing embedded launch stability while adding `--jsonl`
**What goes wrong:** `--embedded --parent-hwnd ...` launches that worked in Phase 44 start failing after parser changes.
**Why it happens:** The parser currently ignores unknown flags; a careless rewrite can accidentally tighten unrelated behavior.
**How to avoid:** Extend `mdcad_launch_config_t` incrementally and keep the current embedded validation order intact.
**Warning signs:** `embed_launch_config_test` failures or embedded launches failing without `--jsonl`.

### Pitfall 2: Fatal file-open failures in `sokol_main()`
**What goes wrong:** A bad JSONL path exits before the child window is usable.
**Why it happens:** It is tempting to verify the file during parse.
**How to avoid:** Parse only the argument contract in `app_launch_config.h`; let the startup controller own file-open/import failure reporting after init.
**Warning signs:** Missing-file launches exit with code 2 instead of showing an in-app error.

### Pitfall 3: Ticking the startup job only when the hierarchy panel is visible
**What goes wrong:** Startup import appears to hang or never begins in embedded viewer-first layout.
**Why it happens:** Current flat import progress is updated inside `ui_scene_hierarchy_draw()`, which itself is inside `if (state.ui_visible)`.
**How to avoid:** Tick from `app.c` every frame before world/render updates.
**Warning signs:** Import starts only after opening/toggling Scene Hierarchy.

### Pitfall 4: Using `last_status` as the only failure surface
**What goes wrong:** Import fails, but the user only sees a green status line in a docked panel or nothing at all.
**Why it happens:** `ui_scene_hierarchy.h` currently draws `last_status` with a success color regardless of content.
**How to avoid:** Add a controller-owned error latch and draw a red app-level status surface.
**Warning signs:** Embedded launch ends on an empty scene with no obvious failure clue.

### Pitfall 5: Accidentally enabling live refresh
**What goes wrong:** Startup imports begin observing file changes by default.
**Why it happens:** Reusing flat observer metadata without fixing `link_enabled=false`.
**How to avoid:** Always stamp observer contract with the source path but keep link OFF in Phase 45.
**Warning signs:** Imported content begins auto-refreshing without an explicit Phase 46 flag.

### Pitfall 6: Partial cleanup assumptions on job error
**What goes wrong:** Planning assumes `jsonl_import_job_reset()` rolls back all partial scene mutations after an error.
**Why it happens:** `reset()` clears job buffers/state; it is not a transactional scene rollback helper.
**How to avoid:** Do not promise new rollback semantics in Phase 45. Surface failure clearly and keep implementation scoped to startup orchestration.
**Warning signs:** Tasks start adding broad scene-clear or refresh redesign work that roadmap explicitly deferred.

## Code Examples

Verified patterns from current source:

### Existing flat import submit sequence
```c
// Source: src/ui/ui_scene_hierarchy.h:2485-2500
bool started = jsonl_import_job_start(
    &state->jsonl_flat_import_job,
    state->jsonl_flat_import_path,
    scale,
    state->jsonl_flat_use_colours,
    default_colour,
    state->jsonl_flat_shift_to_com,
    rot_x, rot_y, rot_z
);

if (state->jsonl_flat_has_mesh_data) {
    jsonl_import_job_set_mesh_mode(&state->jsonl_flat_import_job, state->jsonl_flat_mesh_import_mode);
}
jsonl_import_job_set_observer_contract(&state->jsonl_flat_import_job,
                                       state->jsonl_flat_link_file_for_refresh,
                                       state->jsonl_flat_import_path);
```

### Current frame seam where startup tick belongs
```c
// Source: src/app.c:2065-2230
simgui_new_frame(...);

// Phase 45 startup controller should run here, before world/render updates.

if (state.ui_visible) {
    ui_scene_hierarchy_draw(&state.scene_hierarchy);
}

ecs_world_progress(&state.ecs_world, dt);
ecs_scene_update(&state.ecs_scene);
jsonl_observer_system_tick(&state.ecs_scene, scene_solver_now_ms(), &state.selection);
jsonl_observer_tick_flat_refreshes(&state.ecs_scene);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Flat JSONL import starts from Scene Hierarchy menu/popup state and is ticked from the panel draw path | Startup import should be app-owned and frame-driven, while still calling the same `jsonl_import_job_*` APIs | Phase 45 | Removes hidden-panel dependency and keeps embedded launch deterministic. |
| Parser only knows embedded flags and ignores `--jsonl` | Parser should explicitly understand `--jsonl <absolute-path>` while keeping existing embedded validation stable | Phase 45 | Gives a clear launch contract without introducing IPC or sample-host coupling. |
| Scene Hierarchy `last_status` is the main import status surface | Startup import should expose status/error through an app-level overlay independent of dock layout | Phase 45 | Prevents silent blank embedded launches on import failure. |

**Deprecated/outdated for this phase:**
- Scene-Hierarchy-only startup control: insufficient for embedded viewer-first startup.
- Fatal file verification in CLI parse: conflicts with JSON-03.

## Open Questions

1. **Should startup use the existing tiny-file sync shortcut or always tick asynchronously?**
   - What we know: `jsonl_import_job_should_sync()` uses `< 5` lines today.
   - What's unclear: whether Phase 45 wants identical tiny-file behavior or a fully uniform first-frame async controller.
   - Recommendation: keep the existing sync threshold helper for behavior parity, but start the controller in `frame()` so large-file launches stay responsive.

2. **Where should the failure banner be drawn?**
   - What we know: `last_status` in Scene Hierarchy is insufficient and panel-local.
   - What's unclear: whether to place the banner near the viewport or as a small fixed app window.
   - Recommendation: choose the smallest non-modal ImGui window in `app.c`; avoid modifying viewer layout persistence for this phase.

## Risks and Edge Cases

- Quoted Windows paths with spaces must round-trip through `argv` unchanged.
- UNC paths (`\\\\server\\share\\file.jsonl`) should count as absolute if Phase 45 uses the same Windows absolute-path rule as `imgui_storage.h`.
- Relative paths should fail fast as a launch-contract error; nonexistent absolute paths should not.
- Very small JSONL files may complete before the first visible progress update; success/error text still needs to latch correctly.
- Startup import must not alter Phase 44 embedded focus, close, or layout persistence behavior.

## Phase-Specific Out of Scope

- Launch-time live refresh flag and observe-on-start behavior (`JSON-02`, `JSON-04`) — Phase 46
- Sample host resolution/passing of a bundled JSONL path (`HOST-02`+) — Phase 47
- IPC, in-process embedding, or host-driven import control
- Redesign of flat import root/entry hierarchy, observer semantics, or transactional refresh behavior

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Native build / test target generation | ✓ | 4.3.2 | — |
| CTest | Targeted native regression runs | ✓ | 4.3.2 | — |
| Node.js | GSD tooling / auxiliary repo scripts | ✓ | v25.9.0 | — |
| .NET SDK | Avalonia sample-host build/inspection | ✓ | 10.0.102 | — |
| Ninja | Optional fast local generator | ✗ | — | Use existing `build-vulkan` Visual Studio generator / `cmake --build build-vulkan` |

**Missing dependencies with no fallback:**
- None for this phase research/planning path.

**Missing dependencies with fallback:**
- `ninja` — use the existing checked-in/generated `build-vulkan` tree and `ctest --test-dir build-vulkan`.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CTest + native C harness executables declared in `src/CMakeLists.txt` |
| Config file | `src/CMakeLists.txt` |
| Quick run command | `ctest --test-dir build-vulkan --output-on-failure -R "embed_launch_config_test|jsonl_flat_import_options_contract_test|jsonl_flat_anchor_scoped_ingest_test"` |
| Full suite command | `ctest --test-dir build-vulkan --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| JSON-01 | `--jsonl` accepts one absolute path and arms startup flat import without UI | unit + contract | `ctest --test-dir build-vulkan --output-on-failure -R "embed_launch_config_test|jsonl_startup_import_controller_test"` | `embed_launch_config_test` ✅ / `jsonl_startup_import_controller_test` ❌ Wave 0 |
| JSON-03 | Missing/bad startup JSONL keeps viewer usable and surfaces clear failure state | unit + source contract | `ctest --test-dir build-vulkan --output-on-failure -R "jsonl_startup_import_controller_test|startup_jsonl_app_contract_test"` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `ctest --test-dir build-vulkan --output-on-failure -R "embed_launch_config_test|jsonl_flat_import_options_contract_test|jsonl_flat_anchor_scoped_ingest_test"`
- **Per wave merge:** `ctest --test-dir build-vulkan --output-on-failure -R "embed_launch_config_test|jsonl_flat_import_options_contract_test|jsonl_flat_anchor_scoped_ingest_test|jsonl_reparse_transaction_test|jsonl_flat_observer_manual_refresh_test"`
- **Phase gate:** Full suite green before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] Extend `src/tests/embed_launch_config_test.c` — cover valid absolute `--jsonl`, duplicate `--jsonl`, missing value, and relative-path rejection.
- [ ] `src/tests/jsonl_startup_import_controller_test.c` — header-level controller tests for start success, async tick completion, missing-file failure, and parse-error failure without UI dependencies.
- [ ] `src/tests/startup_jsonl_app_contract_test.c` — source contract proving `app.c` ticks startup import outside `ui_scene_hierarchy_draw()` / `state.ui_visible`.

## Sources

### Primary (HIGH confidence)
- `src/app_launch_config.h` — existing embedded launch parse contract
- `src/app.c` — authoritative startup/init/frame lifecycle and embedded event ownership
- `src/jsonl_import_job.h` — flat import job state machine, sync threshold, observer contract capture
- `src/jsonl_loader.h` — flat JSONL quick-scan and file-open error behavior
- `src/ui/ui_scene_hierarchy.h` — current flat import UI workflow and status handling
- `src/components/jsonl_observer_comp.h` — observer metadata defaults and message storage
- `src/jsonl_observer_system.h` — current linked-refresh separation from import
- `src/CMakeLists.txt` — actual test targets and CTest integration
- `src/tests/embed_launch_config_test.c` — parser test seam
- `src/tests/jsonl_flat_import_options_contract_test.c` — flat import option contract seam
- `src/tests/jsonl_flat_anchor_scoped_ingest_test.c` — flat import hierarchy contract seam
- `samples/avalonia-host/MainWindow.axaml.cs` — current host launch arguments

### Secondary (MEDIUM confidence)
- `build-vulkan/CTestTestfile.cmake` via `ctest -N --test-dir build-vulkan` — confirms current runnable native test inventory

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - All recommendations reuse current repo-local seams rather than external assumptions.
- Architecture: HIGH - Control-flow recommendation is based directly on current `sokol_main()`, `init()`, `frame()`, and flat import job ownership.
- Pitfalls: MEDIUM - Most are directly visible in source, but partial-error cleanup behavior remains an inferred risk area not yet covered by dedicated tests.

**Research date:** 2026-05-15
**Valid until:** 2026-06-14
