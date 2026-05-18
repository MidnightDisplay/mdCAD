# Phase 50 Manual Checklist

Purpose: keep the existing Windows diagnostic host as the authoritative proof surface while the internal backend seam is extracted.

## Automated Preflight

1. Run the Phase 50 regression preflight:
   `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadSessionCoordinatorTests|FullyQualifiedName~MdCadLaunchSnapshotTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadRuntimeResolverTests"`
   - Result: [ ] PASS [ ] FAIL [ ] BLOCKED
   - Notes:
2. Build the diagnostic host:
   `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
   - Result: [ ] PASS [ ] FAIL [ ] BLOCKED
   - Notes:
3. Confirm the copied runtime bundle is present under the host output:
   `Test-Path "samples/avalonia-host/bin/Release/net10.0-windows10.0.19041.0/mdcad-runtime/mdCAD.exe"`
   - Expected output: `True`
   - Result: [ ] PASS [ ] FAIL [ ] BLOCKED
   - Notes:

## Launch Commands

- Diagnostic default:
  `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`
- Sealed mode:
  `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release -- --presentation-mode sealed`

## Checklist

### 1. Runtime bundle presence

- In the running host, confirm the runtime line starts with:
  `runtime: copied bundle present ->`
- Confirm the path points at:
  `samples/avalonia-host/bin/Release/net10.0-windows10.0.19041.0/mdcad-runtime/mdCAD.exe`
- Confirm the harness line starts with:
  `harness: mode=diagnostic; AutoStart=true; live refresh=off; jsonl: example resolved ->`
- Result: [ ] PASS [ ] FAIL [ ] BLOCKED
- Notes:

### 2. Diagnostic launch and attach

- Start the host in diagnostic mode.
- If `AutoStart=false`, press **Launch Session**; otherwise wait for the automatic launch.
- If you click **Launch Session**, confirm the harness line ends with:
  `action: StartAsync requested`
- Confirm the embedded mdCAD window attaches inside the host region and remains interactive.
- In the control's diagnostic status area, confirm:
  - `launch: active`
  - `attach: attached`
  - `jsonl: viewer-managed after launch (requested: <resolved example path>)`
  - `live refresh: viewer-managed after launch (off)`
  - `detail: child hwnd attached: 0x...`
- Confirm the proof surface is still `samples/avalonia-host`, not a new backend-only harness.
- Result: [ ] PASS [ ] FAIL [ ] BLOCKED
- Notes:

### 3. Stop and relaunch

- Click **Close Session** and confirm the viewer exits cleanly.
- Confirm the harness line ends with:
  `action: StopAsync requested`
- In diagnostic mode after stop, confirm:
  - `launch: idle`
  - `attach: idle`
  - `detail: No host-owned failure.`
- Before relaunch, run:
  `Get-CimInstance Win32_Process -Filter "Name = 'mdCAD.exe'" | Select-Object ProcessId, ExecutablePath`
  - Expected result before relaunch: no rows
- Click **Launch Session** again and confirm a second attach succeeds in the same host window.
- After relaunch, confirm the harness line ends with:
  `action: StartAsync requested`
- After relaunch, run the same process query and confirm the only running `mdCAD.exe` comes from the copied runtime bundle path.
- Result: [ ] PASS [ ] FAIL [ ] BLOCKED
- Notes:

### 4. Resize continuity

- With an attached session running, resize the host window wider and taller.
- Confirm the embedded mdCAD surface resizes with the placeholder and stays attached.
- Confirm the control still shows:
  - `launch: active`
  - `attach: attached`
- Confirm no attach-loss, blank child, or broken-parent behavior appears after resize.
- Result: [ ] PASS [ ] FAIL [ ] BLOCKED
- Notes:

### 5. Sealed and diagnostic equivalence

- Run the host once in diagnostic mode and once in sealed mode using the commands above.
- In sealed mode, confirm the harness line starts with:
  `harness: mode=sealed; AutoStart=true; live refresh=off; jsonl: example resolved ->`
- Confirm both modes still launch the same embedded runtime and preserve attach, stop, relaunch, and resize behavior.
- Confirm diagnostic mode still exposes the embedded launch/status chrome while sealed mode hides that extra control-owned chrome.
- Result: [ ] PASS [ ] FAIL [ ] BLOCKED
- Notes:

## Result

- [ ] PASS
- [ ] FAIL
- [ ] BLOCKED

Notes:
