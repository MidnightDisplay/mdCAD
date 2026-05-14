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
reported: "no keyboard ownership after click, no text entry fields can be typed in, ctrl+c ctrl+v does not work, ctrl+z for undo does not work, basically everything I tried. Only mouse events get captured by embedded mdCAD"
severity: major

### 2. Scenario 5 — destroyed-parent self-exit
expected: Destroyed-parent mode reports mdCAD self-exit before any host fallback kill, with no save prompt, no standalone fallback, and no surviving mdCAD.exe.
result: pass

### 3. Scenario 7 — embedded layout persistence and standalone isolation
expected: Embedded launches seed the viewer-first layout, persist rearranged panels via build-vulkan\bin\Release\imgui.embedded.ini, and standalone mdCAD continues using imgui.ini independently.
result: issue
reported: "imgui.embdedded.ini does not get updated when I close the host in Step 2. so mdCAD does not fire the save action when host is closed by the user. (I noticed that it does get created when we run this command though: dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release -- --embed-test-mode destroyed-parent, so something is working, but not when the host is closed by the user)"
severity: major

## Summary

total: 3
passed: 1
issues: 2
pending: 0
skipped: 0
blocked: 0

## Gaps
- truth: "One deliberate click gives embedded mdCAD keyboard ownership so text entry, clipboard shortcuts, and undo work inside the hosted viewer."
  status: failed
  reason: "User reported: no keyboard ownership after click, no text entry fields can be typed in, ctrl+c ctrl+v does not work, ctrl+z for undo does not work, basically everything I tried. Only mouse events get captured by embedded mdCAD"
  severity: major
  test: 1
  root_cause: "The Avalonia host never transfers Win32 keyboard focus into the attached mdCAD child HWND after the first click. `samples/avalonia-host/MainWindow.axaml.cs` launches and resizes the child window, but there is no `SetFocus(...)` or equivalent first-click activation path, so mouse input reaches mdCAD while keyboard remains on the Avalonia host."
  artifacts:
    - path: "samples/avalonia-host/MainWindow.axaml.cs"
      issue: "Attach and resize lifecycle code exists, but no child-HWND focus handoff is performed after a deliberate click."
  missing:
    - "Add a deliberate-click keyboard focus handoff to the attached mdCAD child HWND without auto-focusing on attach."
    - "Re-test text entry, clipboard shortcuts, and undo after focus is explicitly transferred to the embedded child."
  debug_session: ""

- truth: "Closing the host normally lets embedded mdCAD flush build-vulkan\\bin\\Release\\imgui.embedded.ini before any fallback cleanup."
  status: failed
  reason: "User reported: imgui.embdedded.ini does not get updated when I close the host in Step 2. so mdCAD does not fire the save action when host is closed by the user. (I noticed that it does get created when we run this command though: dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release -- --embed-test-mode destroyed-parent, so something is working, but not when the host is closed by the user)"
  severity: major
  test: 3
  root_cause: "The normal host-close path waits on the mdCAD process, but it never proactively invalidates/destroys `_placeholderHwnd` the way `destroyed-parent` does. mdCAD therefore often never sees the parent-ended shutdown condition before the host times out and falls back to killing the process, which bypasses the final embedded layout flush."
  artifacts:
    - path: "samples/avalonia-host/MainWindow.axaml.cs"
      issue: "WaitForGracefulExitOnClose() waits for process exit, but does not first destroy or detach the placeholder HWND to trigger mdCAD shutdown."
  missing:
    - "Trigger the same placeholder invalidation path on normal host close before waiting for mdCAD to exit."
    - "Re-run Scenario 7 after host-close invalidation to confirm imgui.embedded.ini is updated on normal user close."
  debug_session: ""
