# Phase 44 Interaction, Focus, Resize, and Orphan Checklist

> Use the exact harness commands below. Record whether teardown came from the mdCAD-side invalid-parent quit path or the host-side `Process.Kill(true)` fallback.

## Scenario 1 - valid attach and resize stress

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`
Observed states:

    pending
    Harness mode: valid
    Attached child HWND: pending
    Parent HWND: pending

Expected status text / visible outcome:
- Status reaches `attached` with `Harness mode: valid`.
- mdCAD attaches inside the placeholder with no standalone fallback.
- Manual resize, snap, maximize/fullscreen, and restore keep the child surface matched to the host region (`INPT-01`).
- The viewport remains visible with no clipped, stale, or offset rendering during resize stress.

Scenario verdict: pending

## Scenario 2 - no auto-focus and first-click ownership

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`
Observed states:

    pending

Expected status text / visible outcome:
- Embedded attach does not auto-focus mdCAD.
- The first deliberate click inside mdCAD activates viewer keyboard ownership.
- `Tab` remains inside mdCAD after activation.
- Clicking a host control or status area returns focus to the host.

Scenario verdict: pending

## Scenario 3 - Alt+Tab or host deactivation cancel

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`
Observed states:

    pending

Expected status text / visible outcome:
- Start an orbit or gizmo drag, then Alt+Tab or deactivate the host app.
- mdCAD cancels the active interaction immediately.
- Returning to the host shows no stuck capture, no stuck drag, and no partially committed edit that cannot be undone.

Scenario verdict: pending

## Scenario 4 - invalid parent HWND failure

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release -- --embed-test-mode invalid-parent`
Observed states:

    pending
    Harness mode: invalid-parent

Expected status text / visible outcome:
- Status reaches `timeout/failure` with `Harness mode: invalid-parent`.
- mdCAD rejects startup quickly because the parent HWND is invalid.
- No standalone fallback window appears.
- The host keeps running and reports the failure state clearly.

Scenario verdict: pending

## Scenario 5 - destroyed parent after attach self-exit

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release -- --embed-test-mode destroyed-parent`
Observed states:

    pending
    Harness mode: destroyed-parent

Expected status text / visible outcome:
- Status reaches `teardown/cleanup` with `Harness mode: destroyed-parent`.
- The placeholder is destroyed after attach and mdCAD exits through the mdCAD-side invalid-parent `sapp_quit()` path.
- No save prompt appears.
- No standalone fallback window appears.
- Confirm there is no surviving mdCAD.exe after teardown.
- Exit origin note: **expected mdCAD-side self-exit**, not host fallback kill.

Scenario verdict: pending

## Scenario 6 - destroy-after-attach placeholder teardown

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release -- --embed-test-mode destroy-after-attach`
Observed states:

    pending
    Harness mode: destroy-after-attach

Expected status text / visible outcome:
- Status reaches `teardown/cleanup` with `Harness mode: destroy-after-attach`.
- The host destroys the placeholder after the child window attaches, without closing the host window.
- No standalone fallback window appears.
- Confirm there is no surviving mdCAD.exe after the placeholder destroy path completes.
- Exit origin note: **expected host-side `Process.Kill(true)` fallback cleanup** if mdCAD has not already exited.

Scenario verdict: pending

## Scenario 7 - embedded layout first-run seed and standalone isolation

Commands:

1. `Remove-Item imgui.embedded.ini -ErrorAction SilentlyContinue; dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`
2. Rearrange the embedded panels, close the host, then run `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release` again.
3. Launch standalone mdCAD with `.\build-vulkan\bin\Release\mdCAD.exe`.

Observed states:

    pending
    Embedded store: imgui.embedded.ini
    Standalone store: imgui.ini

Expected status text / visible outcome:
- The first embedded launch seeds the viewer-first layout: `3D Viewport` centered, `Scene Hierarchy` in the lower-left, and `Entity Inspector`, `Controls`, `Visibility`, plus `Camera Debug` stacked in the lower-right (`INPT-04`).
- Rearranging the embedded panels persists across the second embedded launch through `imgui.embedded.ini`.
- Standalone mdCAD still uses `imgui.ini` and does not inherit or overwrite the embedded layout.

Scenario verdict: pending

Checklist verdict: pending
