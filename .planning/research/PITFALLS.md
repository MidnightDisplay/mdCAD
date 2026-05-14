# Domain Pitfalls

**Project:** mdCAD v1.8 — Embeddable Windows JSONL Viewer  
**Domain:** Windows child-HWND embedding, launch-time JSONL viewing, and mixed host/viewer input ownership  
**Researched:** 2026-05-14  
**Confidence:** HIGH for Win32 lifecycle and process concerns; MEDIUM for host-specific polish

## Critical Launch Blockers

### 1. Reparenting a top-level window instead of creating a true child window
**Risk:** Launching mdCAD normally and calling `SetParent` later can leave styles, activation, clipping, and DPI behavior inconsistent.

**Prevention:**
- Make embedding a dedicated startup mode
- Create the mdCAD window as a child from the start
- Fail fast on invalid parent HWND or incompatible DPI assumptions

**Phase to absorb:** Phase 1

### 2. Treating process spawn as "viewer ready"
**Risk:** The host reports success before the child surface actually exists and is usable.

**Prevention:**
- Separate statuses for:
  1. process launched
  2. child surface attached
  3. first non-zero resize applied
  4. optional JSONL import complete
- Use bounded startup timeouts and explicit failure reporting

**Phase to absorb:** Phase 1

### 3. No explicit focus/input ownership contract
**Risk:** Mouse enters the viewer, but keyboard stays with the host; first click only focuses; shortcuts go to the wrong process.

**Prevention:**
- Define click-to-focus behavior
- Define what happens on focus loss and host re-focus
- Make focus-loss cleanup mandatory for transient interaction state

**Phase to absorb:** Phase 2

### 4. Mouse capture / drag state leaks across host boundaries
**Risk:** Orbit/gizmo drag stays stuck after focus loss, host resize, or mouse-up outside the child region.

**Prevention:**
- Cancel drags on focus loss, capture loss, parent destroy, and hide/minimize
- Explicitly test mouse-down inside / mouse-up outside scenarios

**Phase to absorb:** Phase 2

### 5. Whole-window size assumptions break in embedded mode
**Risk:** Zero-size transitions, rapid resize bursts, or layout churn break the viewport, pick math, or render targets.

**Prevention:**
- Treat zero-size as a valid transient state
- Gate heavy resize work behind actual size changes
- Verify repeated resize with active content

**Phase to absorb:** Phase 2

### 6. Startup JSONL import runs too early
**Risk:** Large import begins before the embed path is stable, producing blank startup or race conditions.

**Prevention:**
- Parse CLI immediately, but delay actual import until:
  - embed mode is validated
  - child surface exists
  - first non-zero size has been observed
- Keep the viewer alive on import failure and surface the error clearly

**Phase to absorb:** Phase 3

### 7. Windows path and quoting bugs
**Risk:** Spaces, backslashes, UNC paths, or quotes break launch-time import.

**Prevention:**
- Accept absolute paths only
- Prefer wide-character argument parsing on the viewer side
- Avoid hand-rolled quoting on the host side
- Log the resolved path and embed HWND

**Phase to absorb:** Phase 3

### 8. Host/control destruction leaves orphan process or invalid child state
**Risk:** Host closes or recreates the control while mdCAD keeps running or renders into a dead parent.

**Prevention:**
- Define ownership clearly
- Treat control destruction/recreation as a first-class lifecycle path
- Test repeated launch/close/reopen cycles

**Phase to absorb:** Phases 1 and 4

### 9. Launch-time live refresh accidentally changes established observer semantics
**Risk:** v1.8 reopens previously fixed large-file observer regressions by changing defaults or bypassing commit-on-success behavior.

**Prevention:**
- Keep live refresh opt-in only
- Reuse the same observer metadata and commit-on-success semantics as the current linked flat import flow
- Validate launch matrix:
  - embed only
  - embed + import
  - embed + import + live refresh
  - invalid/missing/locked file

**Phase to absorb:** Phase 3

---

## Moderate Pitfalls

### Embedded changes leak into normal standalone mode
**Prevention:** Keep embedded mode as an explicit branch and run standalone smoke verification after each embedding phase.

### Sample host becomes demo-only instead of the conformance harness
**Prevention:** Make the sample host validate resize, focus switching, close/relaunch, good/bad JSONL paths, and optional live refresh.

### Poor error surfacing
**Prevention:** Distinguish embed-contract failure from import failure and avoid silent blank-host failure modes.

---

## Phase-Specific Warning Map

| Phase | Main risk | Mitigation |
|-------|-----------|------------|
| 1 — Embed contract + lifecycle | `SetParent`-style shortcut instead of true child-window startup | Dedicated embed mode, parent validation, child-style creation |
| 1 — Startup coordination | Process exists but viewer is not ready | Separate readiness states |
| 2 — Focus/input | First click eaten, keyboard remains with host | Explicit focus policy and cleanup |
| 2 — Resize | Zero-size / resize storms break viewport | Guard size changes and verify repeated resize |
| 3 — CLI import | Quoting/path bugs | Absolute paths, wide-char parsing, clear logging |
| 3 — Live refresh | Semantics drift from v1.7 | Keep opt-in default OFF and reuse existing observer contract |
| 4 — Sample hardening | Host only proves happy path | Make sample the actual validation harness |

---

## Recommended Risk Absorption Order

1. Embed contract, child-window creation, startup/teardown rules
2. Focus, input, resize, and interaction-state correctness
3. Startup import and live refresh wiring
4. Sample host hardening and regression closure

---

## Sources

- `.planning/PROJECT.md`
- `src/app.c`
- `src/ui/ui_viewport.h`
- `src/jsonl_import_job.h`
- `src/jsonl_observer_system.h`
- Microsoft `SetParent`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setparent
- Microsoft `WM_MOUSEACTIVATE`: https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mouseactivate
- Microsoft `SetFocus`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setfocus
- Microsoft `SetCapture`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setcapture
- Microsoft `GetClientRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getclientrect
- Microsoft `WM_SIZE`: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-size
- Microsoft `CreateProcessW`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw
- Microsoft `WaitForInputIdle`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-waitforinputidle
- Microsoft `CommandLineToArgvW`: https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-commandlinetoargvw
- Avalonia `NativeControlHost`: https://api-docs.avaloniaui.net/docs/T_Avalonia_Controls_NativeControlHost
