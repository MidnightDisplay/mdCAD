---
status: partial
phase: 44-embedded-resize-focus-viewer-layout
source:
  - 44-VERIFICATION.md
started: 2026-05-14T20:36:06.680+01:00
updated: 2026-05-14T20:36:06.680+01:00
---

## Current Test

[awaiting human testing]

## Tests

### 1. Scenario 2 — first-click keyboard ownership
expected: Embedded attach does not auto-focus mdCAD; one deliberate click gives mdCAD keyboard ownership; Tab stays in mdCAD; clicking host UI returns focus to the host.
result: [pending]

### 2. Scenario 5 — destroyed-parent self-exit
expected: Destroyed-parent mode reports mdCAD self-exit before any host fallback kill, with no save prompt, no standalone fallback, and no surviving mdCAD.exe.
result: [pending]

### 3. Scenario 7 — embedded layout persistence and standalone isolation
expected: Embedded launches seed the viewer-first layout, persist rearranged panels via build-vulkan\bin\Release\imgui.embedded.ini, and standalone mdCAD continues using imgui.ini independently.
result: [pending]

## Summary

total: 3
passed: 0
issues: 0
pending: 3
skipped: 0
blocked: 0

## Gaps
