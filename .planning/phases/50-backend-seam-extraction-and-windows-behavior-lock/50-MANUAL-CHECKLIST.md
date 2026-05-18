# Phase 50 Manual Checklist

Purpose: keep the existing Windows diagnostic host as the authoritative proof surface while the internal backend seam is extracted.

## Preconditions

1. Build the diagnostic host:
   `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
2. Confirm the copied runtime bundle is present under the host output:
   `samples/avalonia-host/bin/Release/net10.0-windows10.0.19041.0/mdcad-runtime/mdCAD.exe`
3. Launch the host from the repo root with one of these flows:
   - Diagnostic default:
     `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`
   - Sealed mode:
     `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release -- --presentation-mode sealed`

## Checklist

### 1. Runtime bundle presence

- Confirm the host-owned runtime status line starts with:
  `runtime: copied bundle present ->`
- Confirm the resolved path points at the copied output bundle, not a repo-root fallback.

### 2. Diagnostic launch and attach

- Start the host in diagnostic mode.
- If `AutoStart=false`, press **Launch Session**; otherwise wait for the automatic launch.
- Confirm mdCAD attaches inside the embed region and remains interactive.
- Confirm the host continues to use the existing diagnostic harness controls instead of a new proof surface.

### 3. Stop and relaunch

- Click **Close Session** and confirm the viewer exits cleanly.
- Click **Launch Session** again and confirm a second attach succeeds in the same host window.
- Confirm there is no orphaned `mdCAD.exe` left behind after relaunch.

### 4. Resize continuity

- With an attached session running, resize the host window wider and taller.
- Confirm the embedded mdCAD surface resizes with the placeholder and stays attached.
- Confirm no attach-loss or broken-child behavior appears after resize.

### 5. Sealed and diagnostic equivalence

- Run the host once in diagnostic mode and once in sealed mode.
- Confirm both modes still launch the same embedded runtime and preserve attach, stop, relaunch, and resize behavior.
- Confirm diagnostic mode still exposes the extra launch/status chrome while sealed mode remains minimal.

## Result

- [ ] PASS
- [ ] FAIL

Notes:
