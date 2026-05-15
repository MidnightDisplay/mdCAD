# Phase 43 Bootstrap Smoke Checklist

## Scenario 1 - valid attach

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`
Observed states:

    attached
    Harness mode: valid
    Attached child HWND: 0x231D34
    Parent HWND: 0x251470

Embed failure evidence: embeds without failure
Child window result: present
Standalone fallback result: no fallback, mdCAD launched and running embedded
Debug-window baseline: not visible
Scenario verdict: PASS — valid attach works, the requested mdCAD windows are visible, and the child surface now follows manual resize, snap-size, and fullscreen/windowed transitions closely enough to approve.

## Scenario 2 - invalid parent HWND failure

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release -- --embed-test-mode invalid-parent`
Observed states:

    timeout/failure
    Harness mode: invalid-parent
    Embedded startup failed: invalid parent HWND
    Embedded startup failed: invalid parent HWND


Embed failure evidence: see above
Child window result: showing white background in the parent control
Standalone fallback result: fast failure
Debug-window baseline: not visible
Scenario verdict: PASS — invalid parent HWND fails fast with no standalone fallback.

## Scenario 3 - destroyed parent HWND failure

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release -- --embed-test-mode destroyed-parent`
Observed states:

    timeout/failure
    Harness mode: destroyed-parent
    Embedded startup failed: invalid parent HWND
    Embedded startup failed: invalid parent HWND

Embed failure evidence: see above
Child window result: showing black background in the parent control
Standalone fallback result: fast failure
Debug-window baseline: not visible
Scenario verdict: PASS — destroyed parent HWND fails fast with no standalone fallback.

Checklist verdict: PASS
