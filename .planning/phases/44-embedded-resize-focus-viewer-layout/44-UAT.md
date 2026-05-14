---
status: complete
phase: 44-embedded-resize-focus-viewer-layout
source:
  - 44-01-SUMMARY.md
  - 44-02-SUMMARY.md
  - 44-03-SUMMARY.md
  - 44-04-SUMMARY.md
started: 2026-05-14T20:36:06.680+01:00
updated: 2026-05-14T20:36:06.680+01:00
---

## Current Test

[testing complete]

## Tests

### 1. Valid Attach and Resize Stress
expected: Status reaches `attached` with `Harness mode: valid`; mdCAD attaches inside the placeholder with no standalone fallback; resize, snap, maximize/fullscreen, and restore keep the child surface matched to the host region without clipped or stale viewport rendering.
result: pass

### 2. First-Click Keyboard Ownership
expected: Embedded attach does not auto-focus mdCAD; the first deliberate click inside mdCAD activates viewer keyboard ownership; `Tab` remains inside mdCAD after activation; clicking host UI returns focus to the host.
result: issue
reported: "mdCAD does not get keyboard strokes when focused"
severity: major

### 3. Alt+Tab or Host Deactivation Cancel
expected: Starting an orbit or gizmo drag and then deactivating the host app immediately cancels the active interaction with no stuck capture, stuck drag, or partial edit left behind.
result: pass

### 4. Invalid Parent HWND Failure
expected: Status reaches `timeout/failure` with `Harness mode: invalid-parent`; mdCAD rejects startup quickly because the parent HWND is invalid; no standalone fallback window appears and the host stays alive with a clear failure message.
result: pass

### 5. Destroyed-Parent Self-Exit
expected: Status reaches `teardown/cleanup` with `Harness mode: destroyed-parent`; destroying the placeholder after attach exits mdCAD through the mdCAD-side invalid-parent quit path, with no save prompt, no standalone fallback, and no surviving process.
result: issue
reported: "Destroyed-parent mode destroyed the placeholder after attach and is waiting for the mdCAD invalid-parent quit path. Host fallback cleanup completed (exit code -1); no surviving mdCAD.exe."
severity: major

### 6. Destroy-After-Attach Fallback Cleanup
expected: Status reaches `teardown/cleanup` with `Harness mode: destroy-after-attach`; the host destroys the placeholder after attach, no standalone fallback window appears, and host-side fallback cleanup leaves no surviving mdCAD.exe.
result: pass

### 7. Embedded Layout Persistence and Standalone Isolation
expected: The first embedded launch seeds the viewer-first layout; rearranging embedded panels persists across relaunches through `imgui.embedded.ini`; standalone mdCAD keeps using `imgui.ini` without inheriting or overwriting the embedded layout.
result: issue
reported: "embedded mdCAD always launched the default viewer-first layer, it does not read nor does it create imgui.embedded.ini. the rearranged state is not persisted."
severity: major

## Summary

total: 7
passed: 4
issues: 3
pending: 0
skipped: 0
blocked: 0

## Gaps

- truth: "The first deliberate click inside mdCAD activates viewer keyboard ownership and mdCAD receives embedded keyboard shortcuts such as Tab."
  status: failed
  reason: "User reported: mdCAD does not get keyboard strokes when focused"
  severity: major
  test: 2
  root_cause: ""
  artifacts: []
  missing: []
  debug_session: ""

- truth: "Destroyed-parent mode exits through the mdCAD-side invalid-parent quit path before host fallback cleanup is needed."
  status: failed
  reason: "User reported: Destroyed-parent mode destroyed the placeholder after attach and is waiting for the mdCAD invalid-parent quit path. Host fallback cleanup completed (exit code -1); no surviving mdCAD.exe."
  severity: major
  test: 5
  root_cause: ""
  artifacts: []
  missing: []
  debug_session: ""

- truth: "Embedded launches persist rearranged layout through imgui.embedded.ini and reload that embedded layout without affecting standalone imgui.ini."
  status: failed
  reason: "User reported: embedded mdCAD always launched the default viewer-first layer, it does not read nor does it create imgui.embedded.ini. the rearranged state is not persisted."
  severity: major
  test: 7
  root_cause: ""
  artifacts: []
  missing: []
  debug_session: ""
