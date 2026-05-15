# Phase 46: Launch-Time Live Refresh - Research

**Researched:** 2026-05-15  
**Domain:** mdCAD startup JSONL launch contract, linked flat observer reuse, embedded-safe refresh status  
**Confidence:** HIGH

## User Constraints

No `46-CONTEXT.md` exists. Use the explicit phase context from the request as locked constraints:

- Keep launch-time live refresh an explicit opt-in.
- Reuse existing observer metadata and commit-on-success semantics.
- Do not regress Phase 45 startup auto-import or Phase 44 embedded lifecycle behavior.
- Do not add IPC or sample-host workflow wiring yet; that belongs to later work.
- Preserve the current default behavior: startup JSONL import alone must **NOT** enable live refresh.

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| JSON-02 | Developer can opt into live refresh for the startup JSONL import with an explicit command-line flag. | Add a dedicated boolean launch flag, store it in `mdcad_launch_config_t`, pass it through the startup controller, and set `jsonl_import_job_set_observer_contract(..., true, path)` only when the flag is present. |
| JSON-04 | Launch-time live refresh reuses the existing linked flat JSONL observer semantics without changing default refresh behavior for other workflows. | Reuse `JsonlObserverComp`, `jsonl_observer_system_tick()`, `jsonl_observer_tick_flat_refreshes()`, baseline stamping, retry/auto-disable behavior, and flat refresh commit-on-success semantics unchanged. |

## Summary

Phase 45 already created the correct seam. `app.c` owns startup import lifecycle, and the normal frame loop already runs `jsonl_observer_system_tick()` plus `jsonl_observer_tick_flat_refreshes()` after `ecs_scene_update()`. That means Phase 46 does **not** need a second refresh path. It only needs a way for startup import to stamp the imported root as linked/observed when the user explicitly asks for it.

The minimal implementation is: add a valueless `--jsonl-live-refresh` flag, require it to be paired with `--jsonl`, store it on `mdcad_launch_config_t`, pass it into `startup_jsonl_import_controller_t`, and let the controller call `jsonl_import_job_set_observer_contract()` with `link_enabled=true` only for the opt-in case. `jsonl_import_job.h` already creates `JsonlObserverComp`, stamps baseline source metadata, and auto-disables observe safely if baseline arming fails. Existing auto/manual flat refresh tests prove the downstream semantics are already stable.

The only notable UX gap is status visibility. Live refresh state is currently understandable through `ui_entity_inspector.h`, but that requires the linked root to be visible/selected. For startup-linked refresh in embedded viewer-first layouts, a small app-level status surface should cover only the startup root: show refresh-running and latched warning/error states, but do not add a new persistent panel or host IPC.

**Primary recommendation:** Add `--jsonl-live-refresh` as an explicit companion to `--jsonl`, flow it through `mdcad_launch_config_t` and `startup_jsonl_import_controller_t`, reuse the existing observer contract unchanged, and extend the Phase 45 app overlay just enough to expose startup-root refresh running/warning state outside Scene Hierarchy visibility.

## Standard Stack

### Core
| Library / Module | Version | Purpose | Why Standard |
|------------------|---------|---------|--------------|
| `src/app_launch_config.h` | repo @ `3798d8b` (2026-05-15) | Launch parser contract | Already owns all CLI parsing for embedded and startup JSONL flags. |
| `src/startup_jsonl_import_controller.h` | repo @ `3798d8b` | Startup import bridge | Already owns launch-time JSONL arm/start/tick/reset outside UI panels. |
| `src/jsonl_import_job.h` | repo @ `3798d8b` | Flat JSONL import engine | Already stamps observer metadata, baseline state, and completion status. |
| `src/jsonl_observer_system.h` | repo @ `3798d8b` | Linked refresh runtime | Already provides auto refresh, coalescing, retry, and commit-on-success replacement. |

### Supporting
| Library / Module | Version | Purpose | When to Use |
|------------------|---------|---------|-------------|
| `src/components/jsonl_observer_comp.h` | repo @ `3798d8b` | Persisted linked-refresh metadata | Use as the single source of truth for startup-linked refresh state. |
| `src/app.c` | repo @ `3798d8b` | App lifecycle and overlay drawing | Use for launch config ownership, frame ordering, and embedded-safe status. |
| `src/ui/ui_scene_hierarchy.h` | repo @ `3798d8b` | Canonical flat import submit flow | Mirror its observer-contract call; do not re-implement refresh logic. |
| `src/ui/ui_entity_inspector.h` | repo @ `3798d8b` | Existing linked-refresh status vocabulary | Reuse its message model and wording for startup-root app-level status. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `--jsonl-live-refresh` | Generic `--live-refresh` | Reject. Too broad for a phase scoped only to startup JSONL. |
| Existing observer runtime | A startup-specific polling loop | Reject. Duplicates retry/coalescing/commit safety that already exists. |
| Controller sets link flag only | Controller owns refresh polling too | Reject. `app.c` already runs the observer system every frame. |
| App-level startup-root overlay | Scene Hierarchy / Inspector only | Reject for embedded viewer-first launches; those panels may stay hidden. |

**Installation:** None — reuse existing repo code.

## Relevant Code Seams

| File | Why it matters |
|------|----------------|
| `src/app_launch_config.h` | Add the new explicit flag and enforce `--jsonl-live-refresh` requires `--jsonl`. |
| `src/startup_jsonl_import_controller.h` | Extend arm state/options so startup import can set `link_enabled` explicitly. |
| `src/jsonl_import_job.h` | Already converts observer contract into a persisted `JsonlObserverComp` and baseline state. |
| `src/jsonl_observer_system.h` | Already handles auto refresh, coalesced reruns, retry policy, safety disable, and commit-on-success subtree replacement. |
| `src/app.c` | Already arms startup import in `init()`, ticks it before the UI visibility gate, and ticks observer systems later in the same frame. |
| `src/ui/ui_entity_inspector.h` | Already defines the current refresh status strings and manual refresh control surface. |
| `src/tests/embed_launch_config_test.c` | Existing parser seam for new flag/default-off coverage. |
| `src/tests/startup_jsonl_import_controller_test.c` | Best seam for controller opt-in/default-off contract tests. |
| `src/tests/startup_jsonl_app_contract_test.c` | Best seam for app wiring and overlay source-contract checks. |
| `src/tests/jsonl_flat_observer_manual_refresh_test.c` | Proves commit-on-success, fallback selection, manual refresh after auto-disable, and delete/cancel safety. |
| `src/tests/jsonl_flat_observer_auto_safety_test.c` | Proves baseline stamping, idle behavior, auto refresh, coalescing, and retry/auto-disable semantics. |

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── app_launch_config.h                # parse/store new explicit live-refresh flag
├── startup_jsonl_import_controller.h  # startup import + observer-contract bridge
├── jsonl_import_job.h                 # observer metadata + baseline arm on import complete
├── jsonl_observer_system.h            # existing auto/manual flat refresh runtime
├── app.c                              # arm/tick overlay flow
└── ui/ui_entity_inspector.h           # existing refresh wording/status model
```

### Pattern 1: Parse live refresh as a scoped companion flag
**What:** Add a valueless flag dedicated to the startup JSONL workflow.  
**When to use:** Any launch that wants startup auto-open + live refresh.  
**Recommendation:** Use `--jsonl-live-refresh`.

**Contract:**
- `--jsonl <absolute-path>` remains optional.
- `--jsonl-live-refresh` is optional.
- `--jsonl-live-refresh` **requires** `--jsonl`.
- Duplicate `--jsonl-live-refresh` is a parse error.
- `--jsonl` without `--jsonl-live-refresh` keeps current default-off behavior.
- Do not probe file existence at parse time.

**Example:**
```c
// Source: src/app_launch_config.h
if (strcmp(arg, "--jsonl") == 0) {
    ...
}
```

### Pattern 2: Startup controller only selects observer intent
**What:** Keep startup import controller as a thin bridge to `jsonl_import_job_t`.  
**When to use:** Always for launch-time JSONL import.

**Recommendation:**
- Store `bool startup_jsonl_live_refresh` in `mdcad_launch_config_t`.
- Extend controller state so `arm()` remembers both `path` and `live_refresh_enabled`.
- In `startup_jsonl_import_controller_tick()`, change only:
```c
// Source pattern: src/startup_jsonl_import_controller.h + src/jsonl_import_job.h
jsonl_import_job_set_observer_contract(&controller->job,
                                       controller->live_refresh_enabled,
                                       controller->requested_path);
```

This is enough because the import job already persists observer metadata and baseline state.

### Pattern 3: Reuse the existing observer runtime unchanged
**What:** Let the imported startup root enter the same observer pipeline as UI-linked flat imports.  
**When to use:** After a successful opt-in startup import.

**Example:**
```c
// Source: src/app.c:2279-2284
ecs_world_progress(&state.ecs_world, dt);
ecs_scene_update(&state.ecs_scene);
jsonl_observer_system_tick(&state.ecs_scene, scene_solver_now_ms(), &state.selection);
jsonl_observer_tick_flat_refreshes(&state.ecs_scene);
```

No startup-specific refresh loop should be added.

### Pattern 4: Query startup-root observer state from the app overlay
**What:** Keep refresh status understandable even when panel UI is hidden.  
**When to use:** Startup-linked refresh only.

**Recommendation:**
- Capture the successful startup root entity on controller completion.
- In `app.c`, query that root's `JsonlObserverComp` and `jsonl_observer_is_flat_refresh_running(...)`.
- Show overlay only for:
  - startup import running,
  - startup-root refresh currently running,
  - startup-root warning/error latched (baseline disable, retry exhaustion, refresh failure).
- Do **not** add a persistent panel or host IPC in this phase.

### Anti-Patterns to Avoid
- **Do not add a second refresh implementation:** reuse `jsonl_observer_system_tick()` and `jsonl_observer_tick_flat_refreshes()`.
- **Do not make startup live refresh default-on:** controller tests already lock default unlinked behavior.
- **Do not use a generic global flag name:** keep the flag scoped to startup JSONL.
- **Do not move refresh status into Scene Hierarchy ownership:** embedded launches may never show that panel.
- **Do not wire the sample host yet:** roadmap assigns host workflow/status wiring to Phase 47.

## Recommended Plan Split

### Plan 46-01 — CLI and controller contract
- Add `--jsonl-live-refresh` parsing and storage in `mdcad_launch_config_t`.
- Extend controller arm/options and keep default-off behavior locked.
- Add parser/controller tests for opt-in, duplicate flag, and `requires --jsonl`.

### Plan 46-02 — App integration and startup-root status
- Pass the new flag from `state.launch` into startup controller arming.
- Capture startup root identity after successful import.
- Extend the Phase 45 overlay to surface startup-root refresh running/warning state outside hidden panels.

### Plan 46-03 — Regression closure and verification
- Add runtime tests proving startup `--jsonl` alone stays idle on file change.
- Add runtime tests proving opt-in startup import enters existing auto-refresh semantics.
- Re-run focused launch/observer regression suite in the configured Windows test build.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Startup live refresh runtime | A startup-only polling/watch loop | `jsonl_observer_system_tick()` + `jsonl_observer_tick_flat_refreshes()` | Existing code already handles retries, coalescing, and commit-on-success subtree replacement. |
| Startup-linked metadata | A second startup refresh state struct | `JsonlObserverComp` | Existing fields already store path, retry budget, source baseline, and user-visible messages. |
| Import completion semantics | Custom replace/rollback logic | `jsonl_import_job_t` + `jsonl_observer_commit_flat_refresh()` | Existing semantics preserve last-good content until staged refresh commits successfully. |
| Status vocabulary | New ad hoc strings | Current inspector/status wording | Reuse existing refresh language so startup/live-refresh behavior matches manual workflows. |

**Key insight:** Phase 46 is a launch-contract problem, not a refresh-architecture problem.

## Common Pitfalls

### Pitfall 1: Flag is accepted without `--jsonl`
**What goes wrong:** Launch appears valid, but nothing can live-refresh.  
**Why it happens:** The new flag is parsed as an independent bool with no dependency check.  
**How to avoid:** Make `--jsonl-live-refresh` a parse error unless `--jsonl` is present.  
**Warning signs:** Parser tests pass for `--jsonl-live-refresh` alone.

### Pitfall 2: Startup import quietly becomes linked by default
**What goes wrong:** `--jsonl` alone starts observing files, violating Phase 35/45 defaults.  
**Why it happens:** Controller hardcodes `link_enabled=true` or defaults the new bool incorrectly.  
**How to avoid:** Default the launch/config/controller flag to false and keep explicit tests for absent-flag behavior.  
**Warning signs:** File edits trigger refresh after plain `--jsonl`.

### Pitfall 3: A second refresh loop is added
**What goes wrong:** Startup-linked content behaves differently from existing linked flat imports.  
**Why it happens:** Implementation duplicates observer polling instead of reusing `jsonl_observer_system.h`.  
**How to avoid:** Let import completion create the observer; let the existing frame tick handle everything after that.  
**Warning signs:** New startup-only refresh helpers appear in `app.c` or controller code.

### Pitfall 4: Refresh state is only visible in panel UI
**What goes wrong:** Embedded viewer-first launches cannot tell whether live refresh is armed, running, or auto-disabled.  
**Why it happens:** Existing inspector status is root-selection-dependent and panel-local.  
**How to avoid:** Add a minimal app-level startup-root status surface for running/warning states.  
**Warning signs:** Opt-in live refresh works, but bad baseline/retry exhaustion is invisible unless panels are opened.

### Pitfall 5: Wrong test build directory is used
**What goes wrong:** Focused CTest runs appear to miss Phase 45/46 tests.  
**Why it happens:** `build/` currently has a stale CTest manifest; `build-vulkan/` includes the newer startup/embed tests.  
**How to avoid:** Use `build-vulkan -C Release` for focused validation or regenerate `build/` before relying on it.  
**Warning signs:** `ctest --test-dir build -N` does not list startup/embed tests.

## Code Examples

Verified patterns from current source:

### Existing flat import stamps observer intent once
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

jsonl_import_job_set_observer_contract(&state->jsonl_flat_import_job,
                                       state->jsonl_flat_link_file_for_refresh,
                                       state->jsonl_flat_import_path);
```

### Import completion already materializes observer metadata and baseline state
```c
// Source: src/jsonl_import_job.h:865-881
if (job->root_entity != 0 && job->observer_contract.captured) {
    JsonlObserverComp observer = jsonl_observer_comp_default();
    observer.scale = job->scale;
    observer.rotation_x = job->rotation_x;
    observer.rotation_y = job->rotation_y;
    observer.rotation_z = job->rotation_z;
    observer.shift_to_center = job->shift_to_com;
    observer.use_jsonl_colours = job->use_jsonl_colours;
    observer.mesh_import_mode = job->mesh_import_mode;
    observer.linked = job->observer_contract.link_enabled;
    jsonl_observer_comp_set_path(&observer, job->observer_contract.source_path);
    jsonl_import_job_arm_linked_observer_baseline(job, &observer, &observe_disabled_for_safety);
    ecs_world_set_jsonl_observer(w, job->root_entity, &observer);
}
```

### App frame already runs the observer runtime after scene update
```c
// Source: src/app.c:2279-2284
ecs_world_progress(&state.ecs_world, dt);
ecs_scene_update(&state.ecs_scene);
jsonl_observer_system_tick(&state.ecs_scene, scene_solver_now_ms(), &state.selection);
jsonl_observer_tick_flat_refreshes(&state.ecs_scene);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Startup JSONL import always stamps `link_enabled=false` | Startup JSONL should stamp `link_enabled=true` only behind an explicit launch flag | Phase 46 | Enables live refresh without changing default startup behavior. |
| Linked refresh is configured from Scene Hierarchy / Entity Inspector workflows | Startup import can join the same linked-refresh runtime by setting the existing observer contract | Phases 35-45 | No refresh architecture redesign is needed. |
| Startup overlay only reports import progress/error | Startup-linked refresh may need startup-root running/warning visibility outside panel UI | Phase 46 | Keeps embedded viewer-first launches understandable. |

**Deprecated/outdated for this phase:**
- A startup-specific refresh loop.
- Generic/global live-refresh CLI naming.
- Assuming panel-local refresh UI is sufficient for embedded startup workflows.

## Open Questions

1. **Should successful opt-in startup live refresh show a transient “armed” confirmation?**
   - What we know: running/failure states are otherwise invisible outside panel UI.
   - What's unclear: whether a one-shot success notice is worth the extra overlay state.
   - Recommendation: keep it minimal; prioritize running/warning/error visibility over persistent success chrome.

2. **Should Scene Hierarchy cache dirtying be extended for observer-driven refreshes?**
   - What we know: `app.c` explicitly dirties hierarchy after startup import completion, but no equivalent dirty call was found around observer refresh commits.
   - What's unclear: whether current manual/auto refresh workflows already tolerate stale hierarchy cache until other actions occur.
   - Recommendation: validate manually before adding broader cache-invalidation work; do not expand this phase unless the issue reproduces.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Rebuild / reconfigure tests | ✓ | 4.3.2 | — |
| CTest | Focused validation | ✓ | 4.3.2 | — |
| GCC | Alternate native rebuild path | ✓ | 15.2.0 (MSYS2) | — |
| MSVC `cl` on PATH | Rebuilding VS-generated configs from shell | ✗ | — | Use existing configured `build-vulkan` artifacts or install VS Build Tools |
| Ninja | Default fast native generator | ✗ | — | Use existing VS-generated build dirs |
| `build-vulkan` configured CTest tree | Relevant Phase 45/46 tests | ✓ | current | Preferred validation dir |
| `build` configured CTest tree | Relevant Phase 45/46 tests | partial | stale manifest | Regenerate before relying on it |

**Missing dependencies with no fallback:**
- None for focused validation of the already-built `build-vulkan` tree.

**Missing dependencies with fallback:**
- `cl` not on PATH — existing `build-vulkan -C Release` tests still run.
- `ninja` missing — use the existing Visual Studio-generated build directories.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CTest + standalone C test executables |
| Config file | `src/CMakeLists.txt` |
| Quick run command | `ctest --test-dir build-vulkan -C Release --output-on-failure -R "(embed_launch_config_test|startup_jsonl_import_controller_test|startup_jsonl_app_contract_test|jsonl_flat_observer_manual_refresh_test|jsonl_flat_observer_auto_safety_test)"` |
| Full suite command | `ctest --test-dir build-vulkan -C Release --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| JSON-02 | Explicit startup flag enables linked refresh only when requested | unit/integration | `ctest --test-dir build-vulkan -C Release --output-on-failure -R "(embed_launch_config_test|startup_jsonl_import_controller_test)"` | ✅ |
| JSON-04 | Startup-linked refresh reuses existing observer semantics and preserves default-off elsewhere | integration | `ctest --test-dir build-vulkan -C Release --output-on-failure -R "(jsonl_flat_observer_manual_refresh_test|jsonl_flat_observer_auto_safety_test|startup_jsonl_app_contract_test)"` | ✅ |

### Sampling Rate
- **Per task commit:** `ctest --test-dir build-vulkan -C Release --output-on-failure -R "(embed_launch_config_test|startup_jsonl_import_controller_test|startup_jsonl_app_contract_test)"`
- **Per wave merge:** `ctest --test-dir build-vulkan -C Release --output-on-failure -R "(jsonl_flat_observer_manual_refresh_test|jsonl_flat_observer_auto_safety_test|embed_launch_config_test|startup_jsonl_import_controller_test|startup_jsonl_app_contract_test)"`
- **Phase gate:** `ctest --test-dir build-vulkan -C Release --output-on-failure`

### Wave 0 Gaps
- [ ] Extend `src/tests/embed_launch_config_test.c` for `--jsonl-live-refresh` success/failure/default-off cases.
- [ ] Extend `src/tests/startup_jsonl_import_controller_test.c` with one default-off idle-on-change case and one opt-in auto-refresh case.
- [ ] Extend `src/tests/startup_jsonl_app_contract_test.c` if app overlay/root-status wiring is added.
- [ ] Regenerate any stale non-vulkan build tree before using `build/` as the authoritative CTest directory.

## Sources

### Primary (HIGH confidence)
- `src/app_launch_config.h` — current launch parser and `--jsonl` contract
- `src/startup_jsonl_import_controller.h` — Phase 45 startup controller behavior
- `src/app.c` — init/frame/cleanup ownership and startup overlay seam
- `src/jsonl_import_job.h` — observer-contract capture, observer materialization, baseline arming
- `src/jsonl_observer_system.h` — auto/manual flat refresh runtime, retry policy, commit-on-success replacement
- `src/components/jsonl_observer_comp.h` — persisted linked observer state contract
- `src/ui/ui_scene_hierarchy.h` — canonical flat import + link checkbox submit flow
- `src/ui/ui_entity_inspector.h` — current linked-refresh status and manual refresh UX
- `src/tests/embed_launch_config_test.c` — parser contract coverage
- `src/tests/startup_jsonl_import_controller_test.c` — startup controller default behavior
- `src/tests/startup_jsonl_app_contract_test.c` — app-owned startup lifecycle contract
- `src/tests/jsonl_flat_observer_manual_refresh_test.c` — manual/commit-on-success safety coverage
- `src/tests/jsonl_flat_observer_auto_safety_test.c` — auto-observe baseline/retry/coalescing coverage
- `src/CMakeLists.txt` — CTest registration for relevant targets
- `ctest --test-dir build-vulkan -C Release --output-on-failure -R "(startup_jsonl_import_controller_test|startup_jsonl_app_contract_test|jsonl_flat_observer_manual_refresh_test|jsonl_flat_observer_auto_safety_test|embed_launch_config_test)"` — passed on 2026-05-15

### Secondary (MEDIUM confidence)
- None.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - direct source inspection of current repo seams
- Architecture: HIGH - current frame/order/controller/observer code already proves the reuse path
- Pitfalls: HIGH - derived from direct source reading plus passing targeted regression tests

**Research date:** 2026-05-15  
**Valid until:** 2026-05-22
