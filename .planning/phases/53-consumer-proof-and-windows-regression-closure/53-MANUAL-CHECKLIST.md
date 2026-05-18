# Phase 53 Manual Checklist

Purpose: record the authoritative Phase 53 Windows runtime proof on `samples/avalonia-host` without mutating the historical Phase 50 checklist or blurring compile proof with runtime proof.

## Required Automated Prep

1. Complete validation row `53-02-01` first:
   `dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~RuntimeRefresh|FullyQualifiedName~MdCadSessionCoordinatorTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadRuntimeResolverTests"`
   - Result: [ ] PASS [ ] FAIL [ ] BLOCKED
   - Notes:

## Launch Command

- Diagnostic proof host:
  `dotnet run --project .\samples\avalonia-host\AvaloniaHost.csproj -c Release`

## Checklist

### 1. Copied runtime presence

- Launch the diagnostic host with the command above.
- Confirm the runtime line starts with:
  `runtime: copied bundle present ->`
- Confirm the path points at the copied host output runtime bundle under:
  `samples/avalonia-host/bin/Release/net10.0-windows10.0.19041.0/mdcad-runtime/mdCAD.exe`
- Confirm the harness line starts with:
  `harness: mode=diagnostic; AutoStart=true; live refresh=off;`
- Result: [ ] PASS [ ] FAIL [ ] BLOCKED
- Notes:

### 2. First viewer start and attach

- If the session does not auto-start, click **StartAsync** once.
- If you click **StartAsync**, confirm the harness line ends with:
  `action: StartAsync requested`
- Confirm the embedded mdCAD window appears inside the host region.
- Confirm the embedded control status indicates an active, attached session.
- Result: [ ] PASS [ ] FAIL [ ] BLOCKED
- Notes:

### 3. Stop and no-stray-process check

- Click **StopAsync** and confirm the embedded viewer exits cleanly.
- Confirm the harness line ends with:
  `action: StopAsync requested`
- After stop, run:
  `Get-CimInstance Win32_Process -Filter "Name = 'mdCAD.exe'" | Select-Object ProcessId, ExecutablePath`
- Expected result before relaunch: no copied-runtime `mdCAD.exe` remains running for the proof host session.
- Result: [ ] PASS [ ] FAIL [ ] BLOCKED
- Notes:

### 4. Relaunch in the same host window

- Click **StartAsync** again.
- Confirm a second embedded session starts in the same host window.
- Confirm the harness line ends with:
  `action: StartAsync requested`
- Confirm the copied runtime remains the proof surface and the host returns to an attached session state.
- Result: [ ] PASS [ ] FAIL [ ] BLOCKED
- Notes:

## Result

- [ ] PASS
- [ ] FAIL
- [ ] BLOCKED

Notes:
- Record the authoritative Phase 53 runtime proof outcome here after the user completes the checklist.
