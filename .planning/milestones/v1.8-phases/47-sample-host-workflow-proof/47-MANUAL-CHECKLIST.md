# Phase 47 Sample Host Workflow Proof Checklist

> Use the MSVC Vulkan `build-vulkan` path for every native command in this checklist. Record the host-visible status lines exactly as shown by the sample host and note whether session shutdown completed through graceful exit or host fallback cleanup.

## Preflight

1. `cmake --build build-vulkan --config Release`
2. `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
3. `ctest --test-dir build-vulkan -C Release --output-on-failure`

## Scenario 1 - bundled example launch with live refresh off

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`

Steps:

1. Leave `Live refresh` unchecked.
2. Before launch, confirm the host-visible `jsonl:` line shows the **resolved absolute example path**:
   - `jsonl: example resolved -> <absolute-path-to-sample-host-proof.jsonl>`
3. Click `Launch Session`.
4. Confirm the host-visible state transitions stay host-inferred and honest:
   - `launch: launching` then `launch: active`
   - `attach: waiting for child attach` then `attach: attached`
   - `jsonl:` shows the bundled example path and `--jsonl requested`, then moves to `viewer-managed after launch`
   - `live refresh: off`
5. Confirm mdCAD attaches inside the embed surface with no standalone fallback window.

Observed states:

    launch: active
    attach: attached
    jsonl: viewer-managed after launch (requested: C:\dev\mdCAD\samples\avalonia-host\bin\Release\net8.0-windows10.0.19041.0\resources\examples\sample-host-proof.jsonl)
    live refresh: viewer-managed after launch (off)

Expected outcome:
- The host proves HOST-02 before launch by showing the resolved absolute example path.
- The host never claims confirmed import success; after launch it switches to `viewer-managed` wording.
- mdCAD attaches inside the host region as the embedded child window.

Scenario verdict: PASS

## Scenario 2 - bundled example launch with live refresh on

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`

Steps:

1. Check `Live refresh` before launch.
2. Confirm the pre-launch status lines show:
   - `jsonl: example resolved -> <absolute-path-to-sample-host-proof.jsonl>`
   - `live refresh: requested`
3. Click `Launch Session`.
4. Confirm the host continues to report only what it owns:
   - `jsonl:` transitions to `viewer-managed after launch`
   - `live refresh:` transitions to `viewer-managed after launch (requested)`
5. Confirm mdCAD attaches inside the host region as expected.

Observed states:

    launch: active
    attach: attached
    jsonl: viewer-managed after launch (requested: C:\dev\mdCAD\samples\avalonia-host\bin\Release\net8.0-windows10.0.19041.0\resources\examples\sample-host-proof.jsonl)
    live refresh: viewer-managed after launch (requested)

Expected outcome:
- The exact same bundled example path is used.
- Live refresh stays an explicit opt-in and is not enabled unless the checkbox is checked.
- The host status remains host-inferred only; no new IPC/import-confirmation claim appears.

Scenario verdict: PASS

## Scenario 3 - repeated launch, resize, focus, close, and relaunch

Command: `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`

Steps:

1. Launch once with either live refresh state.
2. Resize the host window, maximize/restore it, and confirm the embedded child keeps tracking the host surface.
3. Click the host chrome outside the embed surface and confirm focus returns to the host.
4. Click `Close Session` and wait for the host to report cleanup completion.
5. Click `Launch Session` again from the same host window.
6. Confirm the second session attaches successfully and does not reuse a dead placeholder HWND.

Observed states:

    launch: active
    attach: attached
    jsonl: viewer-managed after launch (requested:  C:\dev\mdCAD\samples\avalonia-host\bin\Release\net8.0-windows10.0.19041. 0\resources\examples\sample-host-proof.jsonl)
    live refresh: viewer-managed after launch (requested)

Expected outcome:
- `Launch Session` works, `Close Session` works, and `Launch Session` works again from the same window.
- Resize/focus behavior continues to follow the already-proven Phase 44 rules.
- The relaunch path proves that the placeholder/native host surface was recreated cleanly.

Scenario verdict: PASS

## Scenario 4 - orphan-process inspection after close

Commands:

1. `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`
2. `Get-Process mdCAD -ErrorAction SilentlyContinue`

Steps:

1. Launch a session and wait for attach.
2. Click `Close Session` and wait for the host to report cleanup complete.
3. Run `Get-Process mdCAD -ErrorAction SilentlyContinue`.
4. Repeat the same check after closing the entire host window.

Observed states:

    All good as expected
    Graceful cleanup

Expected outcome:
- No orphaned `mdCAD.exe` remains after `Close Session`.
- No orphaned `mdCAD.exe` remains after closing the host window.
- Record whether cleanup ended via graceful exit or host fallback cleanup.

Scenario verdict: PASS

## Scenario 5 - bundled example failure path before launch

Commands:

1. `cmake --build build-vulkan --config Release`
2. `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
3. `Rename-Item samples/avalonia-host/bin/Release/net8.0-windows10.0.19041.0/resources/examples/sample-host-proof.jsonl sample-host-proof.jsonl.bak`
4. `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`
5. `Rename-Item samples/avalonia-host/bin/Release/net8.0-windows10.0.19041.0/resources/examples/sample-host-proof.jsonl.bak sample-host-proof.jsonl`

Steps:

1. Temporarily make the copied bundled example unavailable in the built host output.
2. Launch the host.
3. Confirm the host reports the missing example **before** launching mdCAD:
   - `jsonl: example missing`
   - `launch: timeout/failure` (or equivalent host-owned failure state)
4. Confirm no mdCAD child attaches and no standalone fallback window appears.
5. Restore the example file afterward.

Observed states:

    bundled example missing when no file in the folder. all good when it is restored

Expected outcome:
- The host fails fast on the missing bundled example and never guesses a repo-relative path.
- The failure is visible in host-owned status/detail text before any mdCAD process is successfully launched.
- Restoring the example file returns the host to the normal bundled-example flow.

Scenario verdict: PASS

Checklist verdict: PASS
