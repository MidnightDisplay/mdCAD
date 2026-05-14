# Phase 43: Embed Contract & Child-Window Bootstrap - Research

**Researched:** 2026-05-14  
**Domain:** Win32 child-window bootstrap under Sokol + minimal Avalonia host  
**Confidence:** MEDIUM-HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Phase 43 uses a strict embedded launch contract based on `--embedded` and `--parent-hwnd`.
- **D-02:** If embedded arguments are invalid or child-window startup fails, mdCAD must fail fast and exit with a clear error.
- **D-03:** Embedded mode must never silently fall back to a standalone top-level window.
- **D-04:** The minimal Avalonia sample should auto-launch mdCAD as soon as the host control is ready.
- **D-05:** The sample should surface staged bootstrap states: `launching`, `waiting for child attach`, `attached`, and `timeout/failure`.
- **D-06:** Standalone chrome should be trimmed immediately in Phase 43 instead of waiting until Phase 44.
- **D-07:** Embedded-mode defaults should turn OFF the existing debug-window visibility toggles for `Pick Buffer Debug`, `Slot Buffer Debug`, and `FPS Debug`.

### the agent's Discretion
- Accept both `0x`-prefixed hex and decimal parent-HWND values if that does not change the visible launch contract.
- Choose the exact attach-timeout duration and polling cadence, provided the staged bootstrap states remain visible and failures stay explicit.
- Decide the exact trimmed-chrome surface for Phase 43, as long as obvious standalone-heavy UI is reduced immediately and planning for Phase 44 can still refine the viewer-first layout.

### Deferred Ideas (OUT OF SCOPE)
- Startup JSONL launch flags (`--jsonl`, `--jsonl-live-refresh`) remain Phase 45 and Phase 46 work.
- Full embedded resize/focus/input correctness remains Phase 44.
- Bundled example JSONL resolution and full workflow status coverage remain Phase 47 work.
- Rich host controls, richer IPC/status channels, multiple embedded instances, and cross-platform hosting remain out of scope.

</user_constraints>

## Phase Requirements

| ID | Description | Research Support |
|---|---|---|
| EMBD-01 | Launch mdCAD in Windows embedded mode by passing a parent HWND on the command line | Add pre-init launch-config parsing in `sokol_main()` using existing UTF-8 `argc/argv` |
| EMBD-02 | Create and remain inside a true child window under the supplied parent HWND | Add a child-window branch at the Sokol Win32 creation seam; do not rely on late `SetParent` |
| EMBD-03 | Clear startup failure on missing/invalid/incompatible embed args | Validate before window creation and exit non-zero with explicit error text |
| HOST-01 | Minimal Avalonia sample embeds mdCAD inside `NativeControlHost` | Use a placeholder HWND + process launch + child attach polling pattern |

## Summary

Phase 43 is primarily a **pre-window-creation problem**, not a UI or JSONL problem. `src/app.c` currently ignores `argc/argv`, and Sokol creates the Win32 window before `init()`, so embedded-mode decisions must be made before normal startup continues.

The most important technical constraint is Sokol. The current Win32 path creates a top-level window, and `vendors/libsokol/CMakeLists.txt` tracks `master`. Because the repo already carries Sokol-local customization in `vendors/libsokol/sokol.c`, the safest Phase 43 direction is:

1. **Pin Sokol first**
2. **Keep the Win32 embed patch narrow**
3. **Create a true child window from birth**
4. **Never reparent a normal standalone mdCAD window as the final design**

For the host side, `NativeControlHost` should create and own a placeholder HWND immediately, launch mdCAD with that HWND as `--parent-hwnd`, and poll for a child attach by process ID. The host should not block on synchronous attach assumptions.

## Project Constraints

- `copilot-instructions.md`: not present
- No project-local library choice files were found that change the phase direction beyond existing GSD workflow rules

## Standard Stack

| Library / API | Version / State | Purpose | Why it fits |
|---|---|---|---|
| Sokol `sokol_app.h` | **Pin before implementation** | Native app/window bootstrap | Current floating `master` is too unstable for a local embed patch |
| Win32 `CreateWindowExW` | OS API | True child-window creation | Official child-window path |
| Win32 `EnumChildWindows` + `GetWindowThreadProcessId` | OS APIs | Child attach discovery | Deterministic host-side attach detection |
| Avalonia.Desktop | **11.3.x** | Minimal sample host | Matches milestone research and existing host intent |
| .NET `net8.0` | installed locally | Sample host runtime | Good fit for the Avalonia sample without touching mdCAD core |

### Critical verified facts
- Sokol already provides UTF-8 `argc/argv` to `sokol_main()` from the Windows command line, so Phase 43 does **not** need a second custom wide-char parser.
- Current Sokol Win32 creation has no documented parent-HWND field in the normal app descriptor path.
- Microsoft documents `SetParent` caveats around styles and DPI; that makes it a poor final architecture for mdCAD embedding.

## Architecture Patterns

### Recommended plan boundaries

1. **Launch contract + parser**
   - New small parser module owned by native runtime
   - Parse only `--embedded` and `--parent-hwnd` in this phase
   - Validate before returning `sapp_desc`

2. **Win32 embed seam**
   - Thin Win32-specific helper for embedded config and startup state
   - Narrow Sokol Win32 branch for child-window creation

3. **Embedded-mode UI defaults**
   - Apply embedded defaults centrally in `app.c`
   - Trim standalone-heavy chrome immediately
   - Force requested debug windows OFF in embedded mode

4. **Minimal sample host**
   - Separate Avalonia/.NET project
   - `NativeControlHost` + placeholder HWND + process launch + attach polling
   - Status text only, no rich controls

### Likely file touch points

- `src/app.c` — parse/use launch config, embedded defaults, startup failure path
- `src/app_launch_config.h` / `.c` — **new**
- `src/platform/win32_embed.h` / `.c` — **new**
- `src/ui/ui_visibility.h` or embedded init path in `app.c` — apply debug OFF defaults
- `src/ui/ui_viewport.h` or `app.c` frame path — minimum chrome trimming branch if needed
- `vendors/libsokol/CMakeLists.txt` — stop floating `master`
- `vendors/libsokol/sokol.c` and/or local Sokol app override — child-window creation seam
- `samples/avalonia-host/...` — **new** sample host project

### Recommended patterns

#### Parse embed mode before window creation
Use `sokol_main()` to parse and validate embedded-mode flags before returning the app descriptor.

#### Create child window from birth
Prefer a true `WS_CHILD` window in the Win32 creation path over launching a top-level window and reparenting later.

#### Placeholder-HWND NativeControlHost
Let the sample host create a placeholder HWND synchronously, then attach mdCAD under it asynchronously.

#### PID-based attach discovery
Use `EnumChildWindows` and `GetWindowThreadProcessId` instead of sleeps, window-title guessing, or `WaitForInputIdle` as an attach proxy.

## Don't Hand-Roll

| Problem | Don't build | Use instead | Why |
|---|---|---|---|
| Windows argv handling | custom WinMain parser | Sokol-provided UTF-8 `argc/argv` | Already done upstream |
| Attach detection | sleeps or title matching | `EnumChildWindows` + PID filter | Deterministic and explicit |
| Host surface | ad hoc host HWND hacks | `NativeControlHost` + placeholder child HWND | Matches Avalonia lifecycle |
| Standalone-to-child conversion | late `SetParent` style hacks | child creation at `CreateWindowExW` time | Avoids style, DPI, and flash problems |
| Vendor patching | editing transient build `_deps` output | pinned repo-local Sokol source/override | Reproducible across build dirs |

## Common Pitfalls

1. **Late `SetParent` reparenting** — style mismatches, top-level flash, and DPI issues.
2. **Parsing too late** — top-level window already exists before embed validation fails.
3. **Sokol drift across build trees** — floating `master` creates inconsistent patch targets.
4. **NativeControlHost sync/async mismatch** — host expects child HWND immediately.
5. **Treating process spawn or `WaitForInputIdle` as attached** — false readiness signal.
6. **Insufficient chrome trimming** — phase technically works but still looks like a stuffed standalone app.
7. **Debug windows defaulting back ON** — embedded startup violates the requested baseline.

## Code Examples

### Safe `--parent-hwnd` parsing
```c
uintptr_t value = 0;
char* end = NULL;
errno = 0;
value = (uintptr_t)strtoull(arg, &end, 0);
if ((errno != 0) || (end == arg) || (*end != '\0') || (value == 0)) {
    return launch_error("Invalid --parent-hwnd");
}
cfg.parent_hwnd = (HWND)value;
```

### Embedded debug defaults after init
```c
if (state.launch.embedded) {
    state.pick_debug.window_open = false;
    state.slot_buffer_debug.window_open = false;
    state.fps_debug.window_open = false;
}
```

## Environment Availability

| Dependency | Required By | Available | State | Fallback |
|---|---|---:|---|---|
| CMake | native build | ✓ | present | — |
| Visual Studio/MSBuild configured build dirs | Windows native builds | ✓ | existing configured dirs | — |
| `cl.exe` on PATH | direct shell compile | ✗ | not on PATH | use `cmake --build` on configured build dirs |
| .NET SDK | Avalonia sample host | ✓ | installed | — |
| Pinned Sokol revision | reproducible embed patching | ✗ | repo tracks `master` | make pinning a Wave 0 task |

## Validation Architecture

### Test Framework
| Property | Value |
|---|---|
| Framework | CTest + native C test executables |
| Config file | top-level `CMakeLists.txt` + `src/CMakeLists.txt` |
| Quick run command | `ctest --test-dir build-vulkan -C Release --output-on-failure -R embed_launch_config_test` |
| Full suite command | `ctest --test-dir build-vulkan -C Release --output-on-failure` |
| Manual smoke | Avalonia sample host launch + child attach verification |

### Current observed state
- Existing configured Windows-native build dirs are available
- No existing .NET sample host project is present yet
- No existing native-child-window automation exists

### Wave 0 gaps
- `src/tests/embed_launch_config_test.c` — CLI contract parser coverage
- sample-host project files under `samples/avalonia-host/`
- explicit Windows manual smoke checklist for attach/failure paths
- Sokol pin/patch strategy decision before bootstrap implementation

## Open Questions

1. What exact startup-error surface should mdCAD use in addition to stderr, if any?
2. What is the smallest acceptable chrome trim that still satisfies the user's "trim immediately" requirement?
3. Should Sokol be pinned by vendoring a local override or by fixing the FetchContent revision and patching `vendors/libsokol/sokol.c` minimally?

## Sources

### Primary
- `src/app.c`
- `src/ui/ui_visibility.h`
- `src/ui/ui_pick_debug.h`
- `src/ui/ui_slot_buffer_debug.h`
- `src/ui/ui_fps_debug.h`
- `src/ui/ui_viewport.h`
- `vendors/libsokol/CMakeLists.txt`
- `vendors/libsokol/sokol.c`
- Windows APIs: `CreateWindowExW`, `SetParent`, `EnumChildWindows`, `GetWindowThreadProcessId`, `WaitForInputIdle`
- Avalonia `NativeControlHost`

### Secondary
- `.planning/research/STACK.md`
- `.planning/research/ARCHITECTURE.md`
- `.planning/research/PITFALLS.md`
- `.planning/research/SUMMARY.md`
