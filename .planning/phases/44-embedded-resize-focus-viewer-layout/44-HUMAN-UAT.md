---
status: complete
phase: 44-embedded-resize-focus-viewer-layout
source:
  - 44-VERIFICATION.md
started: 2026-05-14T20:36:06.680+01:00
updated: 2026-05-14T23:42:47.2166361+01:00
---

## Current Test

[testing complete]

## Tests

### 1. Scenario 2 — first-click keyboard ownership
expected: Embedded attach does not auto-focus mdCAD; one deliberate click gives mdCAD keyboard ownership; Tab stays in mdCAD; clicking host UI returns focus to the host.
result: pass
reported: "Everything works now."
notes: "Delete/C/Tab/Ctrl+Z all work after one deliberate click, gizmo drags now stick on release, and camera inertia behaves like standalone again."

### 2. Scenario 5 — destroyed-parent self-exit
expected: Destroyed-parent mode reports mdCAD self-exit before any host fallback kill, with no save prompt, no standalone fallback, and no surviving mdCAD.exe.
result: pass

### 3. Scenario 7 — embedded layout persistence and standalone isolation
expected: Embedded launches seed the viewer-first layout, persist rearranged panels via build-vulkan\bin\Release\imgui.embedded.ini, and standalone mdCAD continues using imgui.ini independently.
result: pass

## Summary

total: 3
passed: 3
issues: 0
pending: 0
skipped: 0
blocked: 0

## Gaps
- none
