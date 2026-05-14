---
status: diagnosed
phase: 44-embedded-resize-focus-viewer-layout
source:
  - 44-VERIFICATION.md
started: 2026-05-14T20:36:06.680+01:00
updated: 2026-05-14T22:10:35.838+01:00
---

## Current Test

[testing complete]

## Tests

### 1. Scenario 2 — first-click keyboard ownership
expected: Embedded attach does not auto-focus mdCAD; one deliberate click gives mdCAD keyboard ownership; Tab stays in mdCAD; clicking host UI returns focus to the host.
result: issue
reported: "Some stuff starter to work: text area ctrl+c ctrl+v ctrl+z Tab. The global (non-text focused) mdCAD Tab (gizmo manipulation mode) and Ctrl+Z (Edit->Undo) do not work. Focusing/defocusing the native control works as expected for the keyboard events that work."
severity: major

### 2. Scenario 5 — destroyed-parent self-exit
expected: Destroyed-parent mode reports mdCAD self-exit before any host fallback kill, with no save prompt, no standalone fallback, and no surviving mdCAD.exe.
result: pass

### 3. Scenario 7 — embedded layout persistence and standalone isolation
expected: Embedded launches seed the viewer-first layout, persist rearranged panels via build-vulkan\bin\Release\imgui.embedded.ini, and standalone mdCAD continues using imgui.ini independently.
result: pass

## Summary

total: 3
passed: 2
issues: 1
pending: 0
skipped: 0
blocked: 0

## Gaps
- truth: "One deliberate click gives embedded mdCAD keyboard ownership so text entry, clipboard shortcuts, and undo work inside the hosted viewer."
  status: failed
  reason: "User reported: text entry, clipboard shortcuts, and focus return now work, but the global (non-text focused) mdCAD Tab shortcut and Ctrl+Z undo still do not."
  severity: major
  test: 1
  root_cause: "The Win32 focus handoff is now working, but `src/app.c` still polls embedded global shortcuts with raw `igIsKeyPressed_Bool(...)` checks. In the current docked ImGui focus/ownership model, text widgets can consume their own keys successfully while the global mdCAD shortcut block still misses routed non-text shortcuts such as `Tab` and `Ctrl+Z`. The remaining fix belongs in mdCAD's ImGui shortcut-routing layer, not in the host or Win32 focus seam."
  artifacts:
    - path: "src/app.c"
      issue: "The embedded global shortcut block still uses raw `igIsKeyPressed_Bool(...)` polling for `Tab` and undo/redo instead of routed ImGui shortcut evaluation."
  missing:
    - "Route embedded global shortcuts through ImGui's shortcut-routing API so docked/focused windows can still reach mdCAD-global `Tab` and undo/redo behavior."
    - "Re-test the non-text-focused mdCAD `Tab` shortcut and `Ctrl+Z` after the global shortcut path is routed correctly."
  debug_session: ""
