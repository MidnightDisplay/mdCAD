# Phase 55: Viewport-only startup mode and F camera reset hotkey - Research

**Researched:** 2026-05-18
**Domain:** Avalonia embedded launch plumbing, native launch config, embedded ImGui layout/shortcut behavior
**Confidence:** HIGH

## User Constraints

- Preserve the compile/build-vs-runtime support boundary established in Phases 52-54.
- Keep scope on this new feature; do not reopen unrelated docs or runtime refresh work.
- Avoid broad UI redesign. The goal is hiding non-viewport ImGui UI, not changing rendering architecture unnecessarily.
- Be explicit about risk around docking/layout persistence, embedded mode behavior, and shortcut routing.

## Summary

Phase 55 fits cleanly into seams that already exist. The Avalonia control already captures launch-affecting state through `MdCadLaunchSnapshot.Create(...)`, the Windows backend already serializes that snapshot into process arguments, and native startup already parses flags into `mdcad_launch_config_t` before `state.launch` is copied into `app.c` (`samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs:90,195-198`, `samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs:5-15`, `samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs:430-460`, `src/app.c:2937-2946`, `src/app_launch_config.h:14-19,78-168`).

The least risky viewport-only implementation is to keep the render pipeline unchanged and make startup mode affect only existing UI/layout seams: (1) keep `ui_viewport_draw(&state.viewport);` exactly where it is, (2) keep the existing `state.ui_visible` gate for non-viewport windows, and (3) add a viewport-only branch to embedded layout policy so embedded viewport-only launches do not inherit the normal multi-panel embedded dock seed (`src/app.c:381-414,2161-2180,2303-2314`, `src/embed_layout_state.h:6-50`, `src/ui/ui_viewport.h:54-146`). The main risk is persisted docking state: hiding windows alone is not enough if `imgui.embedded.ini` preserves split panes.

For camera reset, do not add a new input ownership path. Reuse the exact shortcut seam already added for embedded mode in Phase 44: `mdcad_embedded_shortcuts_allowed(io)` plus `mdcad_embedded_shortcut_pressed(..., dockspace_id)` in embedded launches, and the existing standalone raw-key path in the `else` branch. The camera reset implementation itself should call the existing primitive `orbit_camera_reset(&state.camera)` and nothing more (`src/app.c:338-358,2187-2300`, `src/orbit_camera.h:74-84`).

**Primary recommendation:** Add a new control boolean that flows through `MdCadLaunchSnapshot` to a native `--viewport-only` flag, implement viewport-only as “single viewport layout + no extra panel draws” in embedded mode, and wire `F` into the existing standalone/embedded shortcut branches using `orbit_camera_reset`.

## Truth Map

| Question | Finding | Evidence | Confidence |
|---|---|---|---|
| Control property → launch args path | `MdCadEmbeddedControl` rebuilds `_lastLaunchSnapshot` on property changes; `WindowsMdCadEmbedBackend.CreateStartInfo(...)` is the only Windows arg-construction seam. | `MdCadEmbeddedControl.axaml.cs:47-53,90,195-198,258-268`; `WindowsMdCadEmbedBackend.cs:430-460` | HIGH |
| Native parsing seam | `mdcad_launch_config_t` + `mdcad_launch_config_parse(...)` own launch flag parsing and are already covered by `embed_launch_config_test`. | `src/app_launch_config.h:14-19,78-168`; `src/tests/embed_launch_config_test.c:15-201` | HIGH |
| Viewport-only UI seam | Non-viewport windows are already behind `if (state.ui_visible)`, while viewport rendering is always drawn afterward. | `src/app.c:2165-2180,2309-2314` | HIGH |
| Layout persistence risk | Embedded mode uses a dedicated manual store (`imgui.embedded.ini`) plus one-time dock seeding; existing store reuse will override a new viewport-only default unless separately handled. | `src/embed_layout_state.h:6-50`; `src/app.c:381-414,1821-1826`; `src/imgui_storage.h:222-285,403-447,472-497` | HIGH |
| F hotkey seam | Embedded shortcuts already route through ImGui Shortcut API with dockspace ownership; standalone uses raw key polling under `!io->WantCaptureKeyboard`. | `src/app.c:338-358,2187-2300` | HIGH |
| Camera reset primitive | `orbit_camera_reset(...)` already resets distance, angles, target, inertia, and drag state. | `src/orbit_camera.h:74-84` | HIGH |
| Existing UI reset references | Reset buttons already call `orbit_camera_reset`, so keyboard reset should reuse the same primitive. | `src/ui/ui_controls.h:81-88`; `src/ui/ui_camera_debug.h:86-89` | HIGH |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---|---|---|---|
| Avalonia | 11.3.* | StyledProperty-based control API and host-side state capture | The control already exposes launch-affecting settings as styled properties. |
| .NET / `ProcessStartInfo.ArgumentList` | net10.0 | Windows child-process launch argument transport | Existing backend already uses it and tests lock exact argument order/content. |
| `mdcad_launch_config_t` | repo-local | Native startup flag parse contract | Already owns `--embedded`, `--parent-hwnd`, `--jsonl`, and `--jsonl-live-refresh`. |
| cimgui / Dear ImGui | cimgui 1.92.5dock | Dockspace layout and embedded shortcut routing | Existing embedded shortcut fix already depends on routed ImGui shortcuts and dockspace ownership. |

### Supporting
| Library | Version | Purpose | When to Use |
|---|---|---|---|
| `embed_layout_state` | repo-local | Encodes embedded-vs-standalone layout persistence policy | Extend here for viewport-only-specific embedded layout/store behavior. |
| `imgui_storage` | repo-local | Applies automatic/manual ImGui ini persistence | Reuse unchanged if viewport-only gets its own policy/store filename. |
| `embed_input_state` | repo-local | Embedded keyboard ownership reducer | Preserve as-is; do not add new host focus hacks for `F`. |
| `orbit_camera_reset` | repo-local | Canonical camera reset primitive | Call directly for button and hotkey reset behavior. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|---|---|---|
| New boolean launch/property | New enum startup mode | Overkill for one orthogonal toggle; current launch-affecting settings are booleans. |
| `ui_visible` + layout policy adjustment | New viewport-specific render architecture | Unnecessary scope and regression risk; viewport already renders independently. |
| Embedded `igShortcut_ID(...)` seam | Host-side focus forcing or new input bridge | Reopens Phase 44 focus/ownership work and risks regressions. |

**Installation:**
```bash
# No new packages required for Phase 55.
```

**Version verification:** Repository pins Avalonia as `11.3.*` in `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj:16-19`. Test infrastructure currently restores and runs successfully on this machine.

## Architecture Patterns

### Recommended Project Structure
```text
samples/avalonia-mdcad-control/
├── MdCadEmbeddedControl.axaml.cs                 # public StyledProperty seam
└── Host/
    ├── MdCadLaunchSnapshot.cs                    # launch-affecting snapshot record
    └── Windows/WindowsMdCadEmbedBackend.cs       # ProcessStartInfo argument builder

src/
├── app_launch_config.h                           # native CLI/startup parsing
├── app.c                                         # init/layout/draw/shortcut orchestration
├── embed_layout_state.h                          # embedded layout policy reducer
└── tests/                                        # small C contract/reducer tests
```

### Pattern 1: Launch-affecting control state flows through snapshot, not direct backend reads
**What:** Properties are collected into `MdCadLaunchSnapshot`, then start/relaunch logic uses snapshot equality to decide when a relaunch is required.
**When to use:** For any setting that must affect child process startup.
**Example:**
```csharp
// Source: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs
private MdCadLaunchSnapshot CaptureLaunchSnapshot()
{
    return MdCadLaunchSnapshot.Create(JsonlPath, StartupLiveRefreshEnabled);
}
```

### Pattern 2: Embedded viewport behavior belongs in layout policy + existing draw gates
**What:** Embedded mode already uses a dedicated layout policy and separate dock persistence store.
**When to use:** For startup-mode-specific panel visibility/layout changes.
**Example:**
```c
// Source: src/app.c
if (state.ui_visible) {
    ui_controls_draw(&state.controls);
    ui_camera_debug_draw(&state.camera_debug);
    ui_visibility_draw(&state.visibility);
    ui_entity_inspector_draw(&state.entity_inspector);
    ui_scene_hierarchy_draw(&state.scene_hierarchy);
}
ui_viewport_draw(&state.viewport);
```

### Pattern 3: Embedded keyboard shortcuts must go through routed ImGui shortcuts
**What:** Embedded global shortcuts are bound to the dockspace owner via `igShortcut_ID(...)`.
**When to use:** Any non-text global hotkey that should work in embedded mode without host focus hacks.
**Example:**
```c
// Source: src/app.c
static bool mdcad_embedded_shortcut_pressed(ImGuiKeyChord key_chord, ImGuiID owner_id) {
    return igShortcut_ID(
        key_chord,
        ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverFocused,
        owner_id);
}
```

### Anti-Patterns to Avoid
- **Reusing `PresentationMode` for viewport-only:** `PresentationMode` controls host chrome (`Sealed` vs `Diagnostic`), not child viewer startup behavior.
- **Only hiding panel draws without handling layout persistence:** existing `imgui.embedded.ini` can preserve split docks and leave wasted space.
- **Adding a new camera-reset implementation:** use `orbit_camera_reset(...)`; it already resets all relevant camera state.
- **Changing embedded keyboard ownership rules for `F`:** Phase 44 already locked the ownership model; reuse it.

## Don’t Hand-Roll

| Problem | Don’t Build | Use Instead | Why |
|---|---|---|---|
| Launch-setting propagation | Ad-hoc backend property reads | `MdCadLaunchSnapshot` | Existing coordinator relaunch logic already depends on snapshot equality. |
| Native flag parsing | New parser or backend-only interpretation | `mdcad_launch_config_parse(...)` | Existing tests already lock duplicate/missing/invalid argument behavior. |
| Viewport-only rendering mode | Separate render loop / bypass app frame | Existing `state.ui_visible` gate + layout policy | Minimal blast radius; viewport window already renders independently. |
| Camera reset behavior | Partial field resets in shortcut code | `orbit_camera_reset(...)` | Prevents missing inertia/drag-state cleanup. |
| Embedded hotkey routing | Host IPC/focus bridges | `mdcad_embedded_shortcut_pressed(...)` | Existing embed routing is the proven seam. |

**Key insight:** This phase is mostly plumbing and policy, not new architecture. The safest implementation is to extend the already-tested seams instead of inventing new ones.

## Common Pitfalls

### Pitfall 1: Viewport-only still inherits the old embedded dock layout
**What goes wrong:** The app hides other windows, but the viewport does not fill the surface because `imgui.embedded.ini` preserves the normal split layout.
**Why it happens:** Embedded mode currently reuses one manual store filename for all embedded launches.
**How to avoid:** Give viewport-only embedded launches a distinct layout policy/store filename and a one-window seed path.
**Warning signs:** Bottom/side dock regions remain visible on first viewport-only launch after a prior normal embedded run.

### Pitfall 2: Overloading host presentation with viewer startup behavior
**What goes wrong:** `PresentationMode` starts to mean both host chrome and child viewer window composition.
**Why it happens:** Both affect “what the user sees,” but they live on opposite sides of the process boundary.
**How to avoid:** Keep viewport-only as a launch-affecting boolean in snapshot/args/native config.
**Warning signs:** Planner starts touching `MdCadPresentationMode` enum or XAML-only branches instead of snapshot/backend code.

### Pitfall 3: F hotkey breaks embedded text entry or focus ownership
**What goes wrong:** `F` fires from the wrong scope or prompts a refactor of embedded focus logic.
**Why it happens:** Single-letter shortcuts are tempting to implement with raw polling everywhere.
**How to avoid:** Add `F` only inside the existing shortcut block, using the same embedded-vs-standalone split already used for `C`, `Tab`, undo/redo, and delete.
**Warning signs:** Proposed changes touch host focus APIs, Win32 message ownership, or shortcut handling outside `src/app.c`.

### Pitfall 4: Camera reset leaves stale interaction state
**What goes wrong:** View resets visually, but drag/inertia state continues.
**Why it happens:** Shortcut code resets only distance/target and not action state.
**How to avoid:** Call `orbit_camera_reset(&state.camera)` only.
**Warning signs:** Shortcut implementation writes camera fields directly.

## Code Examples

Verified repo patterns:

### Launch snapshot → Windows args
```csharp
// Source: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs
startInfo.ArgumentList.Add("--embedded");
startInfo.ArgumentList.Add("--parent-hwnd");
startInfo.ArgumentList.Add($"0x{placeholderHandle.ToInt64():X}");

if (snapshot.ShouldPassJsonlArgument)
{
    startInfo.ArgumentList.Add("--jsonl");
    startInfo.ArgumentList.Add(snapshot.LaunchJsonlPath!);
}
```

### Embedded shortcut routing seam
```c
// Source: src/app.c
if (mdcad_embedded_shortcuts_allowed(io)) {
    if (state.launch.embedded) {
        if (mdcad_embedded_shortcut_pressed(ImGuiKey_C, dockspace_id)) {
            // routed embedded shortcut
        }
    } else {
        if (igIsKeyPressed_Bool(ImGuiKey_C, false)) {
            // standalone shortcut
        }
    }
}
```

### Canonical camera reset primitive
```c
// Source: src/orbit_camera.h
static inline void orbit_camera_reset(orbit_camera_t* cam) {
    cam->distance = ORBIT_CAM_DEFAULT_DISTANCE;
    cam->azimuth = ORBIT_CAM_DEFAULT_AZIMUTH;
    cam->elevation = ORBIT_CAM_DEFAULT_ELEVATION;
    cam->target = (vec3_t){0.0f, 0.0f, 0.0f};
    cam->velocity_azimuth = 0.0f;
    cam->velocity_elevation = 0.0f;
    cam->current_action = ORBIT_CAM_ACTION_NONE;
    cam->drag_started_in_viewport = false;
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|---|---|---|---|
| Embedded launches always seed/use the normal multi-panel embedded layout | Extend embedded layout policy for startup-mode-specific layout persistence | Current phase recommendation; current baseline from Phase 44 | Prevents viewport-only mode from inheriting empty dock regions. |
| Embedded global shortcuts relied on raw polling and missed routed shortcuts | Embedded shortcuts use `igShortcut_ID(..., RouteGlobal | RouteOverFocused, dockspace_id)` | Phase 44 (`STATE.md` decisions) | `F` should use this seam, not a new ownership model. |
| Camera reset available only through buttons/debug UI | Add keyboard `F` through existing app shortcut block | Phase 55 target | Closes ergonomic gap without touching camera math. |

**Deprecated/outdated:**
- Reopening host-side `SetFocus`/keyboard-ownership logic for one new hotkey: Phase 44 explicitly moved ownership into mdCAD’s own child-window path and kept host-side focus return separate.

## Recommended Plan Split

### Plan 55-01 — Control launch contract and Windows arg flow
**Goal:** Add a new launch-affecting control property and prove it flows into snapshot equality and Windows launch args.

**Likely files**
- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs`
- `samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs`
- `samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs`
- `samples/avalonia-mdcad-control.tests/MdCadLaunchSnapshotTests.cs`
- `samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs`
- `samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs` (only if relaunch-by-snapshot coverage needs one more case)

### Plan 55-02 — Native viewport-only flag, embedded layout policy, and app contract
**Goal:** Parse the new native flag and make embedded viewport-only launches render only the viewport without broad UI changes.

**Likely files**
- `src/app_launch_config.h`
- `src/app.c`
- `src/embed_layout_state.h`
- `src/tests/embed_launch_config_test.c`
- `src/tests/embed_layout_state_test.c`
- `src/tests/viewport_only_app_contract_test.c` (recommended new contract test)
- `src/CMakeLists.txt`

### Plan 55-03 — F camera reset shortcut and validation closure
**Goal:** Add `F` reset in standalone + embedded shortcut paths, keep focus/text-entry behavior stable, and close proof commands.

**Likely files**
- `src/app.c`
- `src/ui/ui_controls.h` (optional hint text only; not required for behavior)
- `src/tests/camera_shortcut_app_contract_test.c` or extend `viewport_only_app_contract_test.c`
- `src/CMakeLists.txt`
- `.planning/phases/55-viewport-only-startup-mode-and-f-camera-reset-hotkey/55-VALIDATION.md` (or equivalent later validation artifact)

## Open Questions

1. **Should viewport-only embedded mode get its own persisted ini store?**
   - What we know: normal embedded mode uses `imgui.embedded.ini`; existing store reuse will override fresh seeds.
   - What’s unclear: whether the team wants viewport-only layout persistence across launches or a forced fresh layout each time.
   - Recommendation: use a separate viewport-only embedded store filename; it is the lowest-risk way to preserve both behaviors.

2. **Should the native flag be supported outside embedded launches?**
   - What we know: parser can support it easily, but user intent is specifically embedded control startup.
   - What’s unclear: whether standalone CLI behavior should be guaranteed/documented now.
   - Recommendation: implement parser support generically if convenient, but scope tests/behavior guarantees to embedded launches unless a standalone requirement appears.

3. **Does diagnostic host chrome need a new status line for viewport-only?**
   - What we know: existing diagnostic chrome shows JSONL/live-refresh request state, but no generic “all launch args” surface.
   - What’s unclear: whether debugging value outweighs extra XAML/UI churn.
   - Recommendation: skip new diagnostic chrome unless implementation/debugging proves it necessary.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|---|---|---|---|---|
| dotnet | Avalonia control tests | ✓ | 10.0.300 | — |
| cmake | Native test/build orchestration | ✓ | 4.3.2 | — |
| ctest | Native contract test execution | ✓ | 4.3.2 | — |
| `build-vulkan` test tree | Fast native validation lane | ✓ | 24 tests discovered | Reconfigure build if tree becomes stale |

**Missing dependencies with no fallback:**
- None found for research/validation commands on this machine.

**Missing dependencies with fallback:**
- None.

## Validation Architecture

### Test Framework
| Property | Value |
|---|---|
| Framework | xUnit (`xunit` 2.5.3, `Microsoft.NET.Test.Sdk` 17.8.0) + CTest-backed C contract tests |
| Config file | `samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj` and `src/CMakeLists.txt` |
| Quick run command | `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj --filter "FullyQualifiedName~MdCadLaunchSnapshotTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadEmbeddedControlTests|FullyQualifiedName~MdCadSessionCoordinatorTests"` |
| Full suite command | `ctest --test-dir build-vulkan -C Release --output-on-failure` plus `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|---|---|---|---|---|
| P55-01 | Control property flows into snapshot and Windows launch args | unit | `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj --filter "FullyQualifiedName~MdCadLaunchSnapshotTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests"` | ✅ |
| P55-02 | Native parser accepts/rejects new viewport-only flag correctly | native unit | `ctest --test-dir build-vulkan -C Release -R embed_launch_config_test --output-on-failure` | ✅ |
| P55-03 | Embedded viewport-only layout/draw policy is locked | native contract | `ctest --test-dir build-vulkan -C Release -R "embed_layout_state_test|viewport_only_app_contract_test" --output-on-failure` | `embed_layout_state_test` ✅ / app contract ❌ Wave 0 |
| P55-04 | `F` resets camera in standalone and embedded shortcut paths without new focus rules | native contract | `ctest --test-dir build-vulkan -C Release -R "camera_shortcut_app_contract_test|viewport_only_app_contract_test" --output-on-failure` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** run the targeted dotnet or ctest command for the seam being edited.
- **Per wave merge:** `ctest --test-dir build-vulkan -C Release -R "embed_launch_config_test|embed_input_state_test|embed_layout_state_test|startup_jsonl_app_contract_test" --output-on-failure` and the targeted `dotnet test` filter above.
- **Phase gate:** full control test project + relevant native contract tests green before `/gsd-verify-work`.

### Wave 0 Gaps
- [ ] `src/tests/viewport_only_app_contract_test.c` — lock viewport-only init/draw/layout policy in `app.c`
- [ ] `src/tests/camera_shortcut_app_contract_test.c` (or equivalent extension) — lock `F` in both embedded and standalone shortcut branches

## Sources

### Primary (HIGH confidence)
- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs` — control property and snapshot capture seam
- `samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs` — launch-affecting snapshot pattern
- `samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs` — Windows launch arg construction
- `src/app_launch_config.h` — native flag parsing contract
- `src/app.c` — init/layout/draw/shortcut orchestration
- `src/embed_layout_state.h` and `src/imgui_storage.h` — embedded persistence policy
- `src/orbit_camera.h`, `src/ui/ui_controls.h`, `src/ui/ui_camera_debug.h` — reset primitive and existing call sites
- `src/tests/embed_launch_config_test.c`, `src/tests/embed_input_state_test.c`, `src/tests/embed_layout_state_test.c`, `src/tests/startup_jsonl_app_contract_test.c` — existing native contract coverage
- `samples/avalonia-mdcad-control.tests/*.cs` — existing control/backend/snapshot coverage
- Executed commands:
  - `ctest --test-dir build-vulkan -C Release -R "embed_launch_config_test|embed_input_state_test|embed_layout_state_test|startup_jsonl_app_contract_test" --output-on-failure` ✅
  - `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj --filter "FullyQualifiedName~MdCadLaunchSnapshotTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadEmbeddedControlTests|FullyQualifiedName~MdCadSessionCoordinatorTests"` ✅
  - `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj --list-tests` ✅
  - `ctest --test-dir build-vulkan -C Release -N` ✅

### Secondary (MEDIUM confidence)
- `.planning/STATE.md` — prior Phase 44-54 decisions that constrain shortcut/focus/runtime-boundary recommendations
- `AGENTS.md` — project stack/convention summary and session continuity notes

### Tertiary (LOW confidence)
- None. This research was repo-local; no unverified web claims are included.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - directly verified from repo code, csproj files, and passing targeted tests.
- Architecture: HIGH - based on current control/backend/native seams already in production code.
- Pitfalls: MEDIUM - persistence/focus risks are strongly implied by existing architecture and prior decisions, but the exact viewport-only persistence choice remains an implementation decision.

**Research date:** 2026-05-18
**Valid until:** 2026-06-17
