# Phase 44 Interaction, Focus, Resize, and Orphan Checklist

> Wave 0 shell. Fill the observed states and verdicts as Phase 44 runtime work lands.

## Scenario 1 - valid attach and resize stress

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`
Observed states:

    pending
    Harness mode: valid
    Attached child HWND: pending
    Parent HWND: pending

Expected status text / visible outcome:
- mdCAD attaches inside the placeholder with no standalone fallback.
- Manual resize, snap, maximize/fullscreen, and restore keep the child surface matched to the host region.
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
- mdCAD rejects startup quickly because the parent HWND is invalid.
- No standalone fallback window appears.
- The host keeps running and reports the failure state clearly.

Scenario verdict: pending

## Scenario 5 - destroyed parent HWND failure

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release -- --embed-test-mode destroyed-parent`
Observed states:

    pending
    Harness mode: destroyed-parent

Expected status text / visible outcome:
- mdCAD exits immediately when the configured parent HWND is gone.
- No save prompt appears.
- No standalone fallback window appears.

Scenario verdict: pending

## Scenario 6 - destroy-after-attach placeholder teardown

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release -- --embed-test-mode destroy-after-attach`
Observed states:

    pending
    Harness mode: destroy-after-attach

Expected status text / visible outcome:
- The host destroys the placeholder after the child window attaches, without closing the host window.
- mdCAD exits immediately, either through the mdCAD-side invalid-parent quit path or the host-side fallback kill path.
- No standalone fallback window appears.
- No surviving `mdCAD.exe` remains after the placeholder destroy path completes.

Scenario verdict: pending

Checklist verdict: pending
