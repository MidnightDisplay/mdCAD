# Phase 44: Embedded Resize, Focus & Viewer Layout - Research

**Researched:** 2026-05-14
**Domain:** Win32 child-HWND embedding, focus/capture lifecycle, embedded ImGui docking persistence
**Confidence:** MEDIUM

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
## Implementation Decisions

### Focus handoff and keyboard ownership
- **D-01:** mdCAD takes full keyboard ownership only after the user clicks inside the embedded surface.
- **D-02:** Once focused, mdCAD keeps keyboard ownership until the user clicks another host control or host window area.
- **D-03:** `Tab` remains an mdCAD shortcut; it does not move focus back to the host UI.
- **D-04:** Embedded attach must not auto-focus mdCAD at launch; focus waits for the first deliberate user click.

### Drag and capture behavior
- **D-05:** Active camera/gizmo drags continue until mouse-up while the host app stays active, even if the pointer leaves the embedded surface or host-window bounds.
- **D-06:** If the host application deactivates, mdCAD cancels the active interaction immediately and clears capture state rather than resuming later.

### Embedded panel layout
- **D-07:** The embedded default dock layout keeps the 3D viewport in the center, docks Scene Hierarchy below it, and stacks Entity Inspector, Controls, Visibility, and Camera Debug to the right of Scene Hierarchy within the same bottom dock region.
- **D-08:** Embedded mode remembers user layout adjustments, but persists them separately from standalone mdCAD layout state.
- **D-09:** The visible embedded panel set carries forward from Phase 43: Scene Hierarchy, Entity Inspector, Controls, Visibility, and Camera Debug remain available; Pick Buffer Debug, Slot Buffer Debug, and FPS Debug stay off by default.

### Shutdown and orphan behavior
- **D-10:** If the host closes or the parent HWND becomes invalid, embedded mdCAD exits immediately with no standalone fallback and no save prompt.

### the agent's Discretion
- Choose the exact Win32/Avalonia hooks used to detect surface activation, host deactivation, and focus return.
- Choose the exact cancellation mechanism for active camera/gizmo drags, as long as capture is cleared immediately on host-app deactivation.
- Choose the exact dock-split proportions and the persistence mechanism for a separate embedded layout store, as long as it remains distinct from standalone layout state.

### Deferred Ideas (OUT OF SCOPE)
## Deferred Ideas

- Launch-time JSONL flags and startup import error UX remain Phase 45 work.
- Launch-time live refresh and observer opt-in remain Phase 46 work.
- Bundled example JSONL resolution plus richer JSONL/live-refresh host status remain Phase 47 work.
- Rich host/global shortcuts or explicit host-side focus-return controls are not required for Phase 44.
- Embedded chrome trimming/polish beyond the current approved panel set remains later polish, not a Phase 44 blocker.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| EMBD-04 | Embedded mdCAD shuts down cleanly when the host/control lifecycle ends or the parent HWND becomes invalid. | Host-side placeholder destroy + process kill path, child-side parent-validity check + `sapp_quit()`, and no standalone fallback. |
| INPT-01 | User can resize the host control and mdCAD resizes its hosted render surface without clipped, stale, or incorrect viewport behavior. | Keep host `MoveWindow(...)` sync, rely on Sokol `WM_SIZE`/`SAPP_EVENTTYPE_RESIZED`, and keep `ui_viewport_draw()` authoritative for render-target resize. |
| INPT-02 | User can click into the embedded viewer and immediately use mdCAD keyboard and mouse interactions without host interference. | Use native child-HWND focus acquisition on first click; do not proxy keyboard from Avalonia; preserve `Tab` in mdCAD. |
| INPT-03 | User can move focus between host UI and mdCAD without stuck capture, stuck drag, or broken input state. | Add explicit unfocus/deactivate cancellation for orbit camera and gizmo drags; clear Sokol capture state instead of waiting only for mouse-up. |
| INPT-04 | User sees mdCAD in a viewer-first embedded layout that fits the hosted region. | Apply an embedded-only default DockBuilder layout and persist it to a separate embedded settings store. |
</phase_requirements>

## Summary

The existing Phase 43 baseline already has the right ownership split for this phase: the Avalonia host owns the placeholder HWND, process launch, attach detection, and rectangle sync; mdCAD owns rendering, input, docking, and child-window behavior inside the embedded HWND. The main missing work is not a new architecture. It is lifecycle hardening at the seams already visible in `src/app.c`, `src/orbit_camera.h`, `src/gizmo/gizmo.h`, `src/imgui_storage.h`, and `samples/avalonia-host/MainWindow.axaml.cs`.

Two findings matter most for planning. First, do **not** build host-side keyboard or mouse proxying. `SetFocus` is same-thread only, so the Avalonia host should not try to force focus into the external mdCAD child window. Let the child HWND gain focus through the user's first click and handle deactivation/cancellation inside mdCAD's Sokol event path. Second, current Sokol Win32 mouse capture is stateful (`capture_mask`) and only clears on button-up; there is no existing `WM_CAPTURECHANGED`/`WM_CANCELMODE` handling in the patched vendor tree. If focus loss/deactivation is implemented only in app code, future drags can stay broken unless the Sokol-side capture state is cleared too.

For layout, the safest path is an embedded-only ImGui settings store plus a one-time DockBuilder layout recipe. Current desktop startup uses `imgui.ini` automatically, so embedded mode would otherwise overwrite standalone layout. Keep the existing dockspace model, but switch embedded mode to manual ini load/save or an embedded-specific filename, then build the approved default dock layout only when no embedded layout state exists yet.

**Primary recommendation:** Keep input/focus ownership native to the child HWND, add explicit cancel/rollback helpers for active interactions on unfocus/deactivate, and separate embedded ImGui layout persistence from standalone `imgui.ini`.

## Standard Stack

### Core
| Library / API | Version | Purpose | Why Standard |
|---------------|---------|---------|--------------|
| Win32 `user32` child-window APIs (`WM_MOUSEACTIVATE`, `WM_SETFOCUS`, `WM_KILLFOCUS`, `WM_ACTIVATEAPP`, `SetCapture`, `ReleaseCapture`, `DestroyWindow`) | OS APIs | Focus, activation, capture, and shutdown semantics for the embedded HWND | This is the authoritative behavior layer under both Avalonia and Sokol on Windows. |
| Sokol App | pinned commit `2356d22badb8b02b5b4fc745216023a2a699d840` | mdCAD window creation, Win32 message pump, focus/capture events, quit protocol | mdCAD already relies on this exact patched vendor path for child-HWND creation. |
| Dear ImGui / cimgui docking | ImGui `1.92.5` | Dockspace, panel placement, ini persistence, DockBuilder APIs | The existing UI is already dockspace-driven; Phase 44 should extend that, not replace it. |
| Avalonia Desktop | resolved `11.3.15` | Host shell, `NativeControlHost`, window lifecycle, placeholder HWND | Already the proven Phase 43 host stack. |

### Supporting
| Library / Module | Version | Purpose | When to Use |
|------------------|---------|---------|-------------|
| `src/orbit_camera.h` | repo-local | Viewport camera drag lifecycle | Add explicit cancel/reset helper for unfocus/deactivation. |
| `src/gizmo/gizmo.h` + `src/app.c` drag snapshots | repo-local | Entity/vertex drag lifecycle and undo commit seam | Add rollback-on-cancel path distinct from mouse-up commit. |
| `src/imgui_storage.h` | repo-local | ImGui settings load/save seam | Extend native desktop behavior to support embedded-specific persistence. |
| `samples/avalonia-host/MainWindow.axaml.cs` | repo-local | Host attach, resize, close, placeholder lifetime | Add parent-invalid/placeholder-destroy cleanup hooks and manual verification surfaces. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Native child-HWND focus | Host-side keyboard forwarding/proxying | Reject. More code, cross-thread focus problems, and it fights the locked first-click ownership policy. |
| Embedded-specific ImGui settings store | Shared `imgui.ini` | Reject. Shared storage will trample standalone window positions and dock layout. |
| Sokol capture-state fix | Host-side manual mouse tracking | Reject. Capture already belongs to the child HWND; duplicating drag state in the host will drift. |

**Version verification:**
- Avalonia resolved version verified locally with `dotnet list samples/avalonia-host/AvaloniaHost.csproj package` → `11.3.15`
- ImGui version verified from `build-vulkan/_deps/libcimgui-src/imgui/imgui.h` → `1.92.5`
- Sokol revision verified from `vendors/libsokol/CMakeLists.txt` → `2356d22badb8b02b5b4fc745216023a2a699d840`

## Architecture Patterns

### Recommended Project Structure
```text
samples/avalonia-host/
├── MainWindow.axaml.cs      # Host lifecycle, placeholder HWND, process cleanup

src/
├── app.c                    # Embedded interaction state, focus/deactivate event handling
├── orbit_camera.h           # Camera cancel/reset helper
├── imgui_storage.h          # Embedded-vs-standalone ini persistence split
├── gizmo/gizmo.h            # Drag lifecycle helper(s)
└── platform/win32_embed.h   # Embedded-mode state and Win32 helpers
```

### Pattern 1: Host owns process + rectangle; child owns input
**What:** Keep the current Phase 43 boundary. Avalonia launches mdCAD, tracks the placeholder HWND, resizes the child window, and kills the process when the host lifecycle ends. mdCAD alone owns keyboard focus, capture, drag state, rendering, and dock layout.
**When to use:** For all embedded sessions in this milestone.
**Example:**
```c
// Source: src/app.c + build-vulkan/_deps/libsokol-src/sokol_app.h
static void event(const sapp_event* ev) {
    if (ev->type == SAPP_EVENTTYPE_SUSPENDED) {
        imgui_storage_mark_should_save();
    }

    // Phase 44 should extend this same choke point for focus/unfocus/deactivate.
    simgui_handle_event(ev);
}
```

### Pattern 2: First-click activation, then native keyboard ownership
**What:** Do not auto-focus on attach. Let the first click activate/focus the child HWND, then keep keyboard ownership until the user clicks elsewhere in the host. `Tab` remains mdCAD-owned because the keyboard stays with the focused child HWND.
**When to use:** Embedded mode only.
**Example:**
```text
// Source: Microsoft Learn + current host/child split
WM_MOUSEACTIVATE -> activate child window without discarding the click
WM_SETFOCUS      -> mdCAD may now accept keyboard shortcuts
WM_KILLFOCUS     -> mdCAD clears transient input state
```

### Pattern 3: Separate commit-on-mouse-up from cancel-on-deactivate
**What:** Keep drag commit and undo creation on normal mouse-up, but add a separate cancel path for host deactivation/unfocus. Camera cancel should clear drag/capture state immediately. Gizmo cancel should restore drag-start snapshots before freeing buffers so partial edits do not become un-undoable.
**When to use:** Whenever `state.gizmo_drag_active` or orbit camera drag state is live and mdCAD loses focus or the host deactivates.
**Example:**
```c
// Source: src/app.c:2077-2431, src/orbit_camera.h:114-191
if (embedded_cancel_requested) {
    orbit_camera_cancel_interaction(&state.camera);   // new helper
    mdcad_cancel_gizmo_drag(true);                    // rollback + free buffers, no undo push
}
```

### Pattern 4: Embedded-only ImGui ini store + first-run DockBuilder layout
**What:** Embedded mode should either set an embedded-specific ini filename (for native desktop) or switch to manual ini load/save. On first embedded run with no existing embedded settings, build the approved dock layout with DockBuilder and persist it separately.
**When to use:** Embedded mode only.
**Example:**
```c
// Source: imgui.h comments + cimgui DockBuilder wrappers
ImGuiIO* io = igGetIO_Nil();
io->IniFilename = NULL; // manual save/load for embedded-specific storage

igLoadIniSettingsFromMemory(embedded_ini_data, 0);

if (io->WantSaveIniSettings) {
    const char* ini = igSaveIniSettingsToMemory(NULL);
    save_embedded_ini(ini);
    io->WantSaveIniSettings = false;
}
```

### Anti-Patterns to Avoid
- **Host-side `SetFocus` into mdCAD:** Microsoft documents `SetFocus` as same-thread/message-queue scoped. The host should not force focus into the external child process.
- **Shared `imgui.ini` for both modes:** This will corrupt standalone layout expectations.
- **Cancel by only releasing capture:** Current mdCAD drag state also lives in orbit camera fields, gizmo mode, and snapshot buffers.
- **Hardcoded per-frame panel rectangles:** mdCAD already uses dockspace + persistence. Use DockBuilder once, not manual geometry every frame.
- **Assuming `WM_DESTROY` will quit mdCAD automatically:** current Sokol WndProc handles `WM_CLOSE`, not `WM_DESTROY`.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Embedded focus handoff | Keyboard forwarding/proxy IPC from Avalonia to mdCAD | Native child-HWND focus and Win32 activation messages | The OS already decides which HWND receives keyboard events; proxying adds bugs and violates the locked boundary. |
| Dock layout persistence | Custom parser/serializer for panel positions | ImGui ini persistence + DockBuilder defaults | ImGui already persists docks, sizes, collapsed state, and tab stacks. |
| Off-surface drag tracking | Host-side mouse polling loop | Existing Sokol `SetCapture` path plus proper cancel/reset | Current child-window stack already captures correctly while active drags run. |
| Shutdown on orphan/invalid parent | “Hope the destroyed parent kills everything” | Explicit host cleanup + child validity check + `sapp_quit()` | Parent destruction alone does not guarantee mdCAD’s process loop exits cleanly. |

**Key insight:** Phase 44 is mostly about using the existing native/message-loop primitives correctly, not inventing new ownership layers.

## Common Pitfalls

### Pitfall 1: Cross-thread focus forcing
**What goes wrong:** The host tries to call `SetFocus(child_hwnd)` after attach or on click.
**Why it happens:** It seems simpler than relying on native activation.
**How to avoid:** Let the first click focus the child naturally; keep any explicit focus calls inside mdCAD’s own thread/window procedure only.
**Warning signs:** First click is eaten, keyboard never lands in mdCAD, or focus behavior differs between debug/release runs.

### Pitfall 2: Clearing Win32 capture without clearing Sokol capture state
**What goes wrong:** After deactivation, the OS capture is gone but Sokol still believes a button is captured (`capture_mask` remains nonzero).
**Why it happens:** Vendor Sokol capture bookkeeping only clears on button-up in the current tree.
**How to avoid:** Patch or extend the Win32 Sokol path to clear capture bookkeeping on focus loss/deactivation/capture-loss; then cancel mdCAD drag state in `app.c`.
**Warning signs:** The next drag never captures properly, mouse-up is missed, or input feels “stuck” after Alt+Tab.

### Pitfall 3: Partial gizmo edits become non-undoable
**What goes wrong:** A drag is mid-flight when the host deactivates, leaving transformed geometry in scene state but no undo command, because undo is only pushed on mouse-up.
**Why it happens:** Current code applies deltas every frame and commits undo only in the normal release path.
**How to avoid:** Add a distinct cancel path that restores drag-start snapshots and frees buffers without pushing undo.
**Warning signs:** Geometry moves after Alt+Tab but Undo does nothing.

### Pitfall 4: Embedded and standalone layouts overwrite each other
**What goes wrong:** The embedded viewer opens with odd standalone panel positions, or standalone mdCAD inherits cramped embedded docking.
**Why it happens:** Native desktop startup currently uses automatic `imgui.ini` persistence.
**How to avoid:** Use embedded-only settings storage (`imgui.embedded.ini` or manual memory-backed save/load).
**Warning signs:** Repro depends on which mode was launched last.

### Pitfall 5: Placeholder destruction is mistaken for clean process exit
**What goes wrong:** The host removes/destroys the placeholder HWND and assumes mdCAD exits by itself.
**Why it happens:** `DestroyWindow` destroys child windows, but current Sokol quit handling is centered on `WM_CLOSE`.
**How to avoid:** Kill the child process from the host on placeholder/window teardown and add a child-side parent-validity exit guard.
**Warning signs:** Orphaned `mdCAD.exe` remains after closing/removing the host surface.

### Pitfall 6: Treating ImGui capture flags as a focus model
**What goes wrong:** Planning assumes `io->WantCaptureKeyboard` means “embedded viewer has Win32 focus.”
**Why it happens:** ImGui capture flags are often misread as OS focus.
**How to avoid:** Track Win32/Sokol focus explicitly; use ImGui capture only to decide whether mdCAD shortcuts should ignore text-entry widgets.
**Warning signs:** Keyboard shortcuts fire before first click, or host controls stop working after mdCAD loses focus.

## Code Examples

Verified patterns from source/docs:

### Win32 capture is already the right off-surface drag primitive
```c
// Source: build-vulkan/_deps/libsokol-src/sokol_app.h:9153-9165
_SOKOL_PRIVATE void _sapp_win32_capture_mouse(uint8_t btn_mask) {
    if (0 == _sapp.win32.mouse.capture_mask) {
        SetCapture(_sapp.win32.hwnd);
    }
    _sapp.win32.mouse.capture_mask |= btn_mask;
}

_SOKOL_PRIVATE void _sapp_win32_release_mouse(uint8_t btn_mask) {
    if (0 != _sapp.win32.mouse.capture_mask) {
        _sapp.win32.mouse.capture_mask &= ~btn_mask;
        if (0 == _sapp.win32.mouse.capture_mask) {
            ReleaseCapture();
        }
    }
}
```

### ImGui already supports manual ini management
```c
// Source: build-vulkan/_deps/libcimgui-src/imgui/imgui.h:1150-1156,2477,2633
ImGuiIO* io = igGetIO_Nil();
io->IniFilename = NULL;          // disable automatic imgui.ini I/O
igLoadIniSettingsFromMemory(data, 0);

if (io->WantSaveIniSettings) {
    const char* ini = igSaveIniSettingsToMemory(NULL);
    save_embedded_layout_ini(ini);
    io->WantSaveIniSettings = false;
}
```

### DockBuilder can seed the embedded default layout
```c
// Source: build-vulkan/_deps/libcimgui-src/cimgui.h:5251-5264
ImGuiID root = igDockBuilderAddNode(igGetID_Str("mdcad.embedded.dockspace"),
                                    ImGuiDockNodeFlags_DockSpace);
igDockBuilderSetNodeSize(root, (ImVec2){(float)sapp_width(), (float)sapp_height()});

ImGuiID bottom = 0, center = root;
igDockBuilderSplitNode(root, ImGuiDir_Down, 0.32f, &bottom, &center);
ImGuiID right = 0, left = bottom;
igDockBuilderSplitNode(bottom, ImGuiDir_Right, 0.45f, &right, &left);

igDockBuilderDockWindow("3D Viewport", center);
igDockBuilderDockWindow("Scene Hierarchy", left);
igDockBuilderDockWindow("Entity Inspector", right);
igDockBuilderDockWindow("Controls", right);
igDockBuilderDockWindow("Visibility", right);
igDockBuilderDockWindow("Camera Debug", right);
igDockBuilderFinish(root);
```

### Sokol quit API is the clean embedded-exit path
```c
// Source: build-vulkan/_deps/libsokol-src/sokol_app.h:815-841
if (state.launch.embedded && !IsWindow(parent_hwnd)) {
    sapp_quit(); // immediate clean app shutdown, cleanup callback still runs
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Shared desktop `imgui.ini` | Per-mode / embedded-specific ImGui settings | Needed in Phase 44 | Prevents embedded layout from trampling standalone usage. |
| Drag teardown only on mouse-up | Explicit cancel path on unfocus/deactivate plus mouse-up commit path | Needed in Phase 44 | Removes stuck capture and partial-edit risk. |
| Resize by host timer alone | Host `MoveWindow(...)` + child `WM_SIZE`/Sokol resize + viewport-driven RT resize | Phase 43 baseline; keep it | The host already has the correct rectangle authority. |

**Deprecated/outdated:**
- Host-side “just call `SetFocus`” for external child windows — wrong thread boundary.
- Assuming destroyed parent HWND alone is a sufficient embedded shutdown strategy — too risky for clean process exit.

## Open Questions

1. **Is default `WM_MOUSEACTIVATE` behavior already sufficient for the first-click policy in the current patched Sokol child window?**
   - What we know: unhandled `WM_MOUSEACTIVATE` reaches `DefWindowProc`, and Microsoft documents `MA_ACTIVATE` / `MA_ACTIVATEANDEAT` behavior.
   - What's unclear: whether the current child/parent chain ever eats the first click in the actual Avalonia host.
   - Recommendation: add a manual checklist item; only patch `WM_MOUSEACTIVATE` if the first click is being discarded.

2. **Should gizmo cancel preserve the last dragged transform or roll back to drag-start?**
   - What we know: current undo is created only on mouse-up, so preserving the partial transform risks non-undoable state.
   - What's unclear: whether the desired UX prefers “freeze current partial move” or “revert cancelled move.”
   - Recommendation: plan rollback as the default because it is safer with the current undo architecture.

3. **Should parent-invalid exit be host-driven, child-driven, or both?**
   - What we know: host already kills the process on `Window.Closed`; current child code has no parent-validity exit guard.
   - What's unclear: whether control teardown without full window close is expected in the sample harness.
   - Recommendation: do both; host kill for immediacy, child poll/guard for belt-and-suspenders correctness.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Windows Win32 desktop APIs | Embedded child HWND, focus/capture, destroy semantics | ✓ | Windows_NT | — |
| .NET SDK | Avalonia sample host build/run | ✓ | 10.0.102 | — |
| CMake | mdCAD native build/test | ✓ | 4.3.2 | — |
| CTest | Native test execution | ✓ | 4.3.2 | — |

**Missing dependencies with no fallback:**
- None found.

**Missing dependencies with fallback:**
- None found.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CTest + native C unit tests; manual Avalonia/Win32 smoke for UI lifecycle |
| Config file | `CMakeLists.txt` / generated `build-vulkan/CTestTestfile.cmake` |
| Quick run command | `ctest --test-dir build-vulkan -C Release --output-on-failure -R embed_launch_config_test` |
| Full suite command | `ctest --test-dir build-vulkan -C Release --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| EMBD-04 | Host/control teardown or invalid parent exits embedded mdCAD with no fallback | manual integration + native helper unit test | `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release` | ❌ Wave 0 |
| INPT-01 | Host resize updates child surface and viewport/render target coherently | manual integration | `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release` | ❌ Wave 0 |
| INPT-02 | First click gives mdCAD input ownership; no auto-focus; `Tab` stays in mdCAD | manual integration + native reducer/unit test | `ctest --test-dir build-vulkan -C Release --output-on-failure -R embed_input_state_test` | ❌ Wave 0 |
| INPT-03 | Deactivation/focus moves do not leave stuck capture or drag state | native unit test + manual Alt+Tab/click-away smoke | `ctest --test-dir build-vulkan -C Release --output-on-failure -R embed_input_state_test` | ❌ Wave 0 |
| INPT-04 | Embedded default layout fits hosted region and persists separately | native unit test for layout-path selection + manual dock smoke | `ctest --test-dir build-vulkan -C Release --output-on-failure -R embed_layout_state_test` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `ctest --test-dir build-vulkan -C Release --output-on-failure -R "embed_launch_config_test|embed_.*_test"`
- **Per wave merge:** `ctest --test-dir build-vulkan -C Release --output-on-failure` + manual Avalonia host resize/focus/Alt+Tab smoke
- **Phase gate:** Full suite green plus a completed Phase 44 manual checklist covering resize, first-click focus, click-away focus return, Alt+Tab cancel, and host-close/orphan checks

### Wave 0 Gaps
- [ ] `src/tests/embed_input_state_test.c` — focus-state reducer, orbit-camera cancel, and gizmo cancel/rollback coverage for `INPT-02` / `INPT-03`
- [ ] `src/tests/embed_layout_state_test.c` — embedded-vs-standalone ini path/selection coverage for `INPT-04`
- [ ] `.planning/phases/44-embedded-resize-focus-viewer-layout/44-MANUAL-CHECKLIST.md` — resize/focus/Alt+Tab/orphan manual evidence for `EMBD-04`, `INPT-01..04`
- [ ] Optional host harness hook for “destroy placeholder without closing host window” so `EMBD-04` can be reproed deterministically

## Sources

### Primary (HIGH confidence)
- `C:\dev\mdCAD\src\app.c` — current embedded defaults, frame/event seams, keyboard shortcut gate, gizmo drag lifecycle
- `C:\dev\mdCAD\src\ui\ui_viewport.h` — viewport hover/click gate and render-target resize behavior
- `C:\dev\mdCAD\src\orbit_camera.h` — drag lifecycle and missing cancel helper seam
- `C:\dev\mdCAD\src\imgui_storage.h` — current persistence behavior and native desktop gap
- `C:\dev\mdCAD\samples\avalonia-host\MainWindow.axaml.cs` — placeholder HWND, attach detection, resize sync, close cleanup
- `C:\dev\mdCAD\build-vulkan\_deps\libsokol-src\sokol_app.h` — Win32 focus events, mouse capture, quit protocol, HWND getter
- `C:\dev\mdCAD\build-vulkan\_deps\libcimgui-src\imgui\imgui.h` — `IniFilename`, `WantSaveIniSettings`, manual ini APIs
- `C:\dev\mdCAD\build-vulkan\_deps\libcimgui-src\imgui\imgui_internal.h` and `cimgui.h` — DockBuilder API availability
- Microsoft Learn — SetCapture: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setcapture
- Microsoft Learn — ReleaseCapture: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-releasecapture
- Microsoft Learn — SetFocus: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setfocus
- Microsoft Learn — WM_MOUSEACTIVATE: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mouseactivate
- Microsoft Learn — WM_ACTIVATEAPP: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-activateapp
- Microsoft Learn — DestroyWindow: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-destroywindow

### Secondary (MEDIUM confidence)
- Avalonia API docs — `NativeControlHost`: https://api-docs.avaloniaui.net/docs/T_Avalonia_Controls_NativeControlHost
- Avalonia API docs — `InputElement.GotFocus`: https://api-docs.avaloniaui.net/docs/E_Avalonia_Input_InputElement_GotFocus
- Avalonia API docs — `InputElement.LostFocus`: https://api-docs.avaloniaui.net/docs/E_Avalonia_Input_InputElement_LostFocus

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - verified from repo source, resolved package versions, and official Win32 docs
- Architecture: MEDIUM - driven by verified source seams, but first-click/focus behavior still needs manual confirmation in the real host
- Pitfalls: HIGH - directly visible in current code paths and official Win32 semantics

**Research date:** 2026-05-14
**Valid until:** 2026-06-13
