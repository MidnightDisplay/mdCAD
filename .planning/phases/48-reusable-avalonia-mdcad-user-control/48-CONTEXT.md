# Phase 48: Reusable Avalonia mdCAD user control - Context

**Gathered:** 2026-05-15
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 48 turns the proven Phase 43-47 Windows embedding seam into a reusable Avalonia control project that external Windows Avalonia apps can reference directly. The control should be droppable into a normal XAML layout container, own the mdCAD child-window/process lifecycle, expose startup configuration through bindable properties, and carry a pinned mdCAD runtime bundle into the consuming app's output so the host app does not need repo-local build paths or manual copy steps.

In scope:
- A reusable Windows-only Avalonia control/library project for cross-repo local project reference
- Bindable control properties for startup configuration, including optional JSONL path and startup live-refresh opt-in
- Auto-launch by default, plus an explicit-start path for host apps that want to delay startup
- Control-managed relaunch when launch-affecting properties change after the viewer is already running
- Automatic copy of a pinned full mdCAD runtime folder into a dedicated subfolder under the consuming app's output
- A default sealed viewer surface with an explicit diagnostic mode that exposes the sample-host controls/status surface
- Clear sealed-mode behavior for missing, unset, or invalid JSONL startup paths

Out of scope:
- NuGet packaging or public package distribution
- Rich host-to-viewer IPC, runtime command channels, or machine-readable status events
- In-process / DLL / SDK embedding; mdCAD remains a separate launched process
- Cross-platform host parity beyond Windows + Avalonia
- Reopening the already-validated child-HWND, focus, and startup JSONL contracts beyond what this reusable control needs to preserve

</domain>

<decisions>
## Implementation Decisions

### Control startup and configuration contract
- **D-01:** The reusable control supports both auto-launch and explicit start.
- **D-02:** Auto-launch is the default behavior when the control is dropped into XAML and configured through properties.
- **D-03:** The owning app configures the control through bindable control properties rather than a constructor-only options object.
- **D-04:** Changing launch-affecting startup properties after mdCAD is already running triggers a control-managed relaunch with the new startup arguments.

### mdCAD runtime payload packaging
- **D-05:** Phase 48 ships a repo-owned pinned/stable mdCAD runtime bundle; the consuming app does not build mdCAD from source during its own build.
- **D-06:** The shipped payload is a full mdCAD runtime folder, not just `mdCAD.exe`.
- **D-07:** The reusable control project copies that full runtime folder into a dedicated subfolder under the consuming app's output directory.
- **D-08:** The control always resolves mdCAD from the copied stable runtime bundle; the consuming app does not provide or override the runtime path.

### Control presentation modes
- **D-09:** The control has two presentation modes: `sealed` and `diagnostic`.
- **D-10:** `sealed` is the default mode.
- **D-11:** `diagnostic` mode is explicit opt-in from the owning app.
- **D-12:** `diagnostic` mode carries over the sample host's controls plus status/failure surface and renders them above the mdCAD native host region.
- **D-13:** `sealed` mode stays a bare viewer surface instead of exposing the sample host's management chrome.

### Optional JSONL startup behavior
- **D-14:** If `JsonlPath` is intentionally unset or empty, the control launches mdCAD normally without `--jsonl` and without showing a warning.
- **D-15:** If `JsonlPath` is set but points to a missing or unreadable file, the control shows a visible warning and still launches an empty usable viewer.
- **D-16:** The control must not silently hide or swallow a bad requested JSONL path.

### the agent's Discretion
- Choose the exact Avalonia property names, types, and change-notification wiring for the reusable control, as long as they remain bindable and support the locked relaunch behavior.
- Choose the exact dedicated runtime-subfolder name and MSBuild copy mechanics, as long as the pinned full runtime folder lands deterministically under the consuming app's output.
- Choose the exact relaunch throttling/debounce behavior for rapid property changes, as long as host apps do not have to manage mdCAD restarts themselves.
- Choose the exact visual treatment of the sealed-mode warning surface, as long as a bad requested JSONL path is visible without turning the sealed control into the full diagnostic chrome.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — Phase 48 roadmap position, v1.8 scope, and the carry-forward no-IPC/Windows-only milestone boundary.
- `.planning/REQUIREMENTS.md` — validated embedding/sample-host requirements and the still-deferred host integration extensions that Phase 48 must not accidentally absorb.
- `.planning/PROJECT.md` — v1.8 product goal, external-host motivation, and milestone-level constraints.
- `.planning/STATE.md` — current phase status plus prior embedding decisions that now constrain the reusable control.

### Upstream embedded baseline
- `.planning/phases/43-embed-contract-child-window-bootstrap/43-CONTEXT.md` — strict `--embedded --parent-hwnd` contract, no-standalone-fallback rule, and minimal-host baseline.
- `.planning/phases/44-embedded-resize-focus-viewer-layout/44-CONTEXT.md` — focus ownership, shutdown/orphan rules, placeholder invalidation, and host-focus return behavior that the reusable control must preserve.

### Sample-host proof baseline
- `.planning/phases/47-sample-host-workflow-proof/47-01-SUMMARY.md` — bundled deployed-output JSONL resolution and explicit `--jsonl` / `--jsonl-live-refresh` pass-through pattern.
- `.planning/phases/47-sample-host-workflow-proof/47-02-SUMMARY.md` — repeatable relaunch controller, host-owned status surface, and cleanup expectations established by the sample host.
- `.planning/phases/47-sample-host-workflow-proof/47-MANUAL-CHECKLIST.md` — verified launch/relaunch/orphan scenarios that the reusable control must preserve in a less scaffolding-heavy form.

### Code anchors
- `samples/avalonia-host/AvaloniaHost.csproj` — current content-copy-to-output pattern that Phase 48 can extend toward a full runtime-bundle copy.
- `samples/avalonia-host/MainWindow.axaml` — current diagnostic/status UI surface and embed-host layout.
- `samples/avalonia-host/MainWindow.axaml.cs` — placeholder HWND creation, child attach detection, relaunch logic, AppContext-based path resolution, and process cleanup behavior to extract/adapt.
- `src/app_launch_config.h` — mdCAD launch contract for `--embedded`, `--parent-hwnd`, `--jsonl`, and `--jsonl-live-refresh`.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `samples/avalonia-host/MainWindow.axaml.cs` `EmbedNativeControlHost` — already encapsulates `NativeControlHost` placeholder HWND creation and destruction on Windows.
- `samples/avalonia-host/MainWindow.axaml.cs` relaunch/cleanup helpers (`RecreateEmbedSurface`, `WaitForGracefulExitOnClose`, `FinalizeTeardownStatus`) — already own repeated child-process lifecycle management that a reusable control can inherit.
- `samples/avalonia-host/MainWindow.axaml.cs` `TryResolveBundledExample(...)` — already uses `AppContext.BaseDirectory` plus copied content to resolve deployed runtime assets rather than repo-relative assumptions.
- `samples/avalonia-host/AvaloniaHost.csproj` — already copies `resources/examples/**` to output with `PreserveNewest`, which is the nearest existing output-packaging seam for a future runtime bundle.

### Established Patterns
- The host side owns the child HWND placeholder, process launch, attach detection, resize sync, and cleanup; mdCAD owns the actual viewer/input/runtime inside the child window.
- mdCAD stays a separate process launched through `ProcessStartInfo.ArgumentList` with `--embedded`, `--parent-hwnd`, optional `--jsonl`, and optional explicit `--jsonl-live-refresh`; no IPC layer has been added.
- Startup JSONL/live-refresh behavior is launch-time only in the current implementation; there is no existing live reconfiguration channel beyond restarting the process with new args.
- The sample host's status surface is intentionally host-owned and switches to `viewer-managed` wording after attach instead of claiming authoritative mdCAD import success.

### Integration Points
- A new reusable Avalonia control/library project that can be referenced from an external Windows Avalonia app without copying the Phase 47 sample-window scaffold directly.
- Bindable control properties that map to mdCAD launch-time arguments and trigger control-managed restarts when relevant values change.
- MSBuild content/targets that move a pinned full mdCAD runtime folder into a dedicated subfolder under the consuming app's output.
- Replacement of `TryResolveMdcadExecutable(...)` repo-root discovery with runtime-bundle resolution rooted in the consuming app's deployed output.
- Optional diagnostic chrome that reuses the existing sample-host status/control idioms while sealed mode keeps a bare embed surface.

</code_context>

<specifics>
## Specific Ideas

- The target consumer is another Windows Avalonia app outside this repo using a cross-repo local project reference.
- The reusable control should be droppable into a normal XAML container such as a grid cell or stack panel.
- The preferred happy path is: host app starts, control is already present in XAML, the host supplies a JSONL path from stored user settings, and mdCAD comes up automatically.
- The user described the two presentation modes as: diagnostic = "all the guts out"; sealed = default.

</specifics>

<deferred>
## Deferred Ideas

- NuGet packaging/distribution is explicitly future work and out of scope for this phase.
- Rich host-to-viewer IPC, machine-readable readiness/import events, and post-launch command channels remain future host integration work.
- Multi-viewer management and broader embedded-host orchestration remain future phases if needed.
- Cross-platform reusable host controls remain out of scope; this phase stays Windows + Avalonia only.

</deferred>

---

*Phase: 48-reusable-avalonia-mdcad-user-control*
*Context gathered: 2026-05-15*
