# Phase 48 Reusable Avalonia Control Checklist

> Run every command from `C:\dev\mdCAD`. Unless a step says otherwise, the observed `launch:`, `attach:`, `jsonl:`, `live refresh:`, and `detail:` lines come from the **diagnostic surface inside `MdCadEmbeddedControl`**, while `runtime:` and `harness:` come from the sample-host chrome above it.

## Preflight

1. `cmake --build build-vulkan --config Release`
2. `ctest --test-dir build-vulkan -C Release --output-on-failure`
3. `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
4. `Test-Path "samples/avalonia-host/bin/Release/net8.0-windows10.0.19041.0/mdcad-runtime/mdCAD.exe"`

Expected:

- The native build/regression lane is green.
- The host build is green.
- `Test-Path` returns `True` for the copied runtime executable under the host output.

Observed:

    PENDING

Status: PENDING

## P48-M01 - sealed auto-start with copied runtime and valid JSONL

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`

Setup:

1. Set `Presentation` to `Sealed`.
2. Leave `AutoStart` checked.
3. Leave `Live refresh` unchecked.
4. Set `Jsonl scenario` to `Bundled example`.

Steps:

1. Confirm the host-visible lines show:
   - `runtime: copied bundle present -> <absolute-path-to-host-output>\mdcad-runtime\mdCAD.exe`
   - `harness: mode=sealed; AutoStart=true; live refresh=off; jsonl: example resolved -> <absolute-path-to-sample-host-proof.jsonl>`
2. Do not click `StartAsync`.
3. Confirm mdCAD launches automatically and attaches inside the viewer region.
4. Confirm **sealed** mode stays bare:
   - no `Launch Session` / `Close Session` buttons inside the control
   - no diagnostic `launch:` / `attach:` / `jsonl:` / `live refresh:` / `detail:` block inside the control
5. Confirm no warning surface is visible.
6. Confirm no standalone fallback window appears.

Expected outcome:

- The control proves copied-runtime launch from the consumer output.
- Auto-start works without an explicit `StartAsync` call.
- Sealed mode remains minimal while still embedding mdCAD correctly.

Observed:

    PENDING

Status: PENDING

## P48-M02 - `AutoStart=false` plus explicit `StartAsync` / `StopAsync` and orphan cleanup

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`

Setup:

1. Set `Presentation` to `Diagnostic`.
2. Uncheck `AutoStart`.
3. Leave `Live refresh` unchecked.
4. Set `Jsonl scenario` to `Bundled example`.

Steps:

1. Confirm the viewer does **not** auto-launch after the settings settle.
2. Click the host `StartAsync` button.
3. Confirm the diagnostic surface reports the honest host-owned transition:
   - `launch: launching` then `launch: active`
   - `attach: waiting for child attach` then `attach: attached`
   - `jsonl: viewer-managed after launch (requested: <absolute-path-to-sample-host-proof.jsonl>)`
   - `live refresh: viewer-managed after launch (off)`
4. Click the host `StopAsync` button.
5. Run `Get-Process mdCAD -ErrorAction SilentlyContinue`.
6. Close the entire host window.
7. Run `Get-Process mdCAD -ErrorAction SilentlyContinue` again.

Expected outcome:

- `AutoStart=false` waits for an explicit host request.
- `StartAsync` launches the session cleanly.
- `StopAsync` tears it down cleanly.
- No orphaned `mdCAD.exe` remains after stop or after closing the host window.

Observed:

    PENDING

Status: PENDING

## P48-M03 - launch-affecting changes coalesce to the newest snapshot only

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`

Setup:

1. Set `Presentation` to `Diagnostic`.
2. Leave `AutoStart` checked.
3. Set `Jsonl scenario` to `Bundled example`.
4. Wait for the initial attach to complete.

Steps:

1. Turn `Live refresh` on.
2. Quickly change `Jsonl scenario` from `Missing file` to `Unreadable directory` without pausing for the first relaunch to settle.
3. Wait for the control to finish reconciling.
4. Confirm the final diagnostic lines reflect only the newest requested snapshot:
   - `jsonl: viewer-managed after launch (requested: <absolute-path-to-resources-examples-directory>)`
   - `live refresh: viewer-managed after launch (requested)`
5. Run `Get-Process mdCAD -ErrorAction SilentlyContinue`.

Expected outcome:

- The control settles on the final `Unreadable directory` request rather than the intermediate `Missing file` request.
- Only one `mdCAD.exe` remains running after the relaunch settles.
- No standalone fallback window appears during the relaunch.

Observed:

    PENDING

Status: PENDING

## P48-M04 - sealed versus diagnostic presentation

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`

Setup:

1. Set `Jsonl scenario` to `Bundled example`.
2. Leave `AutoStart` checked.

Steps:

1. Set `Presentation` to `Sealed`.
2. Confirm the control surface shows only the bare viewer region (plus warning surface if applicable) and does **not** show the diagnostic management chrome.
3. Switch `Presentation` to `Diagnostic`.
4. Confirm the control now shows:
   - `Launch Session`
   - `Close Session`
   - `launch:`
   - `attach:`
   - `jsonl:`
   - `live refresh:`
   - `detail:`
5. Switch back to `Sealed`.
6. Confirm the diagnostic chrome disappears again while the viewer surface remains usable.

Expected outcome:

- `Sealed` stays minimal.
- `Diagnostic` is explicit opt-in.
- Switching presentation modes does not reopen the old sample-host-owned embedding scaffold.

Observed:

    PENDING

Status: PENDING

## P48-M05 - empty, missing, and unreadable `JsonlPath` requests stay honest and usable

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`

Setup:

1. Set `Presentation` to `Sealed`.
2. Leave `AutoStart` checked.

Steps:

1. Set `Jsonl scenario` to `Empty request`.
2. Confirm the host line reads `jsonl: no startup file requested`.
3. Confirm the control shows **no** warning surface and the viewer still launches/attaches normally.
4. Set `Jsonl scenario` to `Missing file`.
5. Confirm the control shows a compact visible warning surface while still leaving a usable viewer region.
6. Set `Jsonl scenario` to `Unreadable directory`.
7. Confirm the control again shows a compact visible warning surface while still leaving a usable viewer region.
8. Switch `Presentation` to `Diagnostic` and confirm the diagnostic `detail:` line stays host-owned wording rather than claiming import success.

Expected outcome:

- Empty requests launch with no warning and no `--jsonl`.
- Missing and unreadable absolute requests stay visibly warned.
- Bad requests do not block a usable viewer launch.
- The host/control never claim authoritative JSONL import success.

Observed:

    PENDING

Status: PENDING

## Checklist Verdict

- Overall status: PENDING
- Notes:

    PENDING
