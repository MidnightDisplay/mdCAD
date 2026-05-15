# Phase 47: Sample Host Workflow Proof - Research

**Researched:** 2026-05-15  
**Domain:** Avalonia sample-host lifecycle proof for embedded mdCAD launch workflows  
**Confidence:** HIGH

## Summary

The current sample host already proves the hard parts of Phase 43-46: it creates a real `NativeControlHost` placeholder, launches `mdCAD.exe` with `--embedded --parent-hwnd`, detects child attach via Win32 child enumeration, keeps the child resized, and closes the child cleanly on host shutdown (`samples/avalonia-host/MainWindow.axaml.cs:223-353,400-686,770-840`). The native side already supports absolute startup JSONL import and optional `--jsonl-live-refresh`, with runtime viewer-owned overlay status inside `mdCAD` (`src/app_launch_config.h:12-19,128-165`, `src/app.c:1702-1777,1907-1912,2156-2182`).

Phase 47 is therefore mostly host glue, not new native viewer behavior. The biggest gaps are: there is no bundled `resources/examples` folder today, the host never passes `--jsonl` or `--jsonl-live-refresh`, the host UI only exposes coarse attach/failure text, and the host has no in-window relaunch/close-session loop for repeated lifecycle proof. The cleanest finish is to keep the no-IPC boundary, add a copied-on-build example JSONL, expose a tiny host-owned session state model, and explicitly label JSONL/live-refresh text as **launch-requested / host-inferred** while leaving actual import/refresh truth to the existing mdCAD overlay.

**Primary recommendation:** Keep Phase 47 to two implementation slices: (1) bundled example path resolution + launch argument wiring, then (2) host session controls/status for relaunch/close proof using the existing graceful teardown seam and no new IPC.

## Project Constraints (from copilot-instructions.md)

- Treat `gsd-*` work as GSD workflow-driven work.
- After completing the deliverable, offer the next step to the user.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| HOST-02 | Sample host resolves a bundled example JSONL from a relative `resources/examples` folder and launches mdCAD with its absolute path. | Resource-copy pattern, absolute-path resolution, existing `--jsonl` contract, and host launch wiring recommendations below. |
| HOST-03 | Sample host shows launch, attach, JSONL, and live-refresh status text for the embedded session. | Host-side session state model, status-boundary guidance, and no-IPC inference rules below. |
| HOST-04 | Sample host can repeatedly launch, resize, focus, and close the embedded mdCAD session without leaving orphaned processes. | Recreate-placeholder recommendation, teardown reuse guidance, and repeated-cycle validation checklist below. |
</phase_requirements>

## Current State / Gap Analysis

| Requirement | Current State | Gap | Confidence |
|------------|---------------|-----|------------|
| HOST-02 | Host resolves `mdCAD.exe` from `build-vulkan` and launches only `--embedded --parent-hwnd ...` (`samples/avalonia-host/MainWindow.axaml.cs:688-726,307-315`). No `samples/avalonia-host/resources/examples` folder exists. | Add bundled example content, resolve it from the deployed host output, convert to absolute path, and append `--jsonl <absolute-path>`. | HIGH |
| HOST-03 | Host status is limited to `launching`, `waiting for child attach`, `attached`, `teardown/cleanup`, `timeout/failure` plus freeform detail text (`samples/avalonia-host/MainWindow.axaml.cs:151-155,301-352,430-445,473-686`). | Add explicit host-visible JSONL/live-refresh fields. Do **not** claim confirmed import success without IPC; report launch request / example resolution / live-refresh mode / viewer-managed runtime boundary. | HIGH |
| HOST-04 | Resize sync, host-chrome focus return, graceful close wait, and orphan cleanup are already in place from Phase 44 (`samples/avalonia-host/MainWindow.axaml.cs:489-686,770-840`). | Current host is single-launch-per-window. It has no launch/close/relaunch controls, and current teardown destroys the placeholder HWND, so repeated in-window relaunch needs an explicit placeholder recreation/reset seam. | HIGH |

## Standard Stack

### Core
| Library / Tool | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Avalonia | 11.3.15 | Minimal Windows host UI | Already used by the sample; sufficient for `NativeControlHost` + simple controls. |
| .NET | 8.0-windows10.0.19041.0 target | Host runtime | Already configured in `AvaloniaHost.csproj`; no stack expansion needed. |
| Win32 P/Invoke | repo-owned | Placeholder HWND, child enumeration, resize sync | Existing host already depends on this seam; Phase 47 should extend it, not replace it. |
| mdCAD CLI launch contract | repo-owned | `--embedded --parent-hwnd --jsonl --jsonl-live-refresh` | Already implemented and tested in native code. |

### Supporting
| Library / Tool | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| CTest | 4.3.2 | Native regression/smoke | Keep using for parser/startup-controller/native contract checks. |
| CMake `build-vulkan` | existing repo build | Authoritative Windows native build | Use for all Phase 47 validation; this supersedes the temporary MinGW workaround. |
| DispatcherTimer | Avalonia built-in | Attach polling / resize monitoring | Reuse for session state and watchdog timing; already in host. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Copied `resources/examples/*.jsonl` content files | Embedded resource extracted to temp file | Worse: mdCAD needs a real absolute path, and extraction adds needless lifecycle/state complexity. |
| Host-local inferred status text | New IPC/status channel | Out of scope for v1.8 and unnecessary for Phase 47. |
| Manual host UAT + build smoke | UI automation stack | Overkill for this minimal harness; cross-process HWND behavior remains manual anyway. |

**Version verification:** Resolved locally from `samples/avalonia-host/obj/project.assets.json` after a successful `dotnet build`; Avalonia packages resolved to `11.3.15`. Native validation passed in `build-vulkan` (`ctest`: 24/24 green).

## Recommended Plan Split

### Slice 1 — Bundled example launch contract
**Why first:** HOST-02 is the smallest missing behavior and creates the data needed for HOST-03 text.

Deliver:
- Add `samples/avalonia-host/resources/examples/<approved-example>.jsonl`.
- Mark it as content copied to output; resolve from `AppContext.BaseDirectory`, not repo-root assumptions.
- Add host launch options for the bundled example and a minimal live-refresh toggle.
- Extend `ProcessStartInfo.Arguments` to include `--jsonl <absolute-path>` and optional `--jsonl-live-refresh`.
- Fail before process launch if the example cannot be resolved.

### Slice 2 — Session controller and repeated lifecycle proof
**Why second:** HOST-03 and HOST-04 both depend on a host-owned session model, and relaunch is where stale-process/orphan bugs usually reappear.

Deliver:
- Add explicit host controls: `Launch`, `Close Session`, and a live-refresh toggle/label.
- Reuse the existing graceful teardown path before every relaunch.
- Recreate or replace the placeholder HWND after session close so a fresh child can attach.
- Track separate status lines for launch, attach, JSONL, live refresh, and teardown.
- Keep focus policy unchanged: host regains focus via Avalonia; mdCAD claims focus on first click.

## Architecture Patterns

### Recommended Project Structure
```text
samples/avalonia-host/
├── resources/
│   └── examples/           # Bundled JSONL proof fixtures copied next to host output
├── MainWindow.axaml        # Minimal workflow UI: surface + small control/status area
├── MainWindow.axaml.cs     # Session controller, placeholder lifecycle, process launch
└── AvaloniaHost.csproj     # Content-copy rules for example JSONL
```

### Pattern 1: Copy example JSONL as deploy-time content
**What:** Keep the example as a real file under `resources/examples` and copy it into the host output tree.
**When to use:** Always for HOST-02; mdCAD already requires an absolute filesystem path.
**Example:**
```csharp
// Source basis: samples/avalonia-host/MainWindow.axaml.cs:705-726
string examplePath = Path.GetFullPath(
    Path.Combine(AppContext.BaseDirectory, "resources", "examples", "sample.jsonl"));
if (!File.Exists(examplePath))
{
    // fail in host before launching mdCAD
}
```

### Pattern 2: Keep status text host-owned and explicitly inferred
**What:** Model host-visible status as facts the host can actually observe.
**When to use:** For HOST-03 without adding IPC.
**Recommended fields:**
- **Launch:** idle / launching / started / failed / close requested / cleanup complete
- **Attach:** waiting for child attach / attached / attach timeout / child lost
- **JSONL:** example missing / example resolved / `--jsonl` requested / viewer-managed import state
- **Live refresh:** off / requested on / viewer-managed after attach

**Boundary rule:** Do not display `import succeeded` or `live refresh healthy` unless Phase 47 also adds a verified host-readable signal. Today the host can only prove that it resolved the file and passed the CLI flag; mdCAD’s in-view overlay remains the runtime source of truth.

### Pattern 3: Relaunch only after full session reset
**What:** Every new embedded launch must start from a fresh placeholder + fresh process object.
**When to use:** For HOST-04 relaunch/close cycles.
**Example:**
```csharp
// Source basis: existing teardown in MainWindow.axaml.cs:473-686, 828-840
await CloseCurrentSessionAsync();   // graceful wait, then fallback cleanup if needed
RecreateEmbedPlaceholder();         // required because current teardown destroys the HWND
LaunchEmbeddedViewer();
```

### Anti-Patterns to Avoid
- **Do not add IPC for status text:** Phase 47 is a harness phase, not HOSTX-02.
- **Do not resolve the example from repo-root only:** the built sample must work from its own output directory.
- **Do not force focus into the external child HWND from the host:** Phase 44 explicitly moved focus ownership into mdCAD’s native click path.
- **Do not reuse a destroyed placeholder HWND for relaunch:** current teardown invalidates/destroys it.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| JSONL import pipeline | Host-side importer or parser | Existing `--jsonl` startup controller in mdCAD | Native viewer already owns flat-import defaults, error overlay, and observer linkage. |
| Live-refresh engine | Host file watcher | Existing `--jsonl-live-refresh` opt-in | Phase 46 already proved startup-linked refresh reuse. |
| Cross-process focus forcing | `SetFocus`/`SetActiveWindow` on mdCAD child from host | Existing Phase 44 first-click focus ownership | Cross-process focus forcing was already identified as unreliable. |
| Runtime import confirmation | Fake host success heuristics | Explicit “requested / viewer-managed” status boundary | Without IPC, the host cannot authoritatively know import completion. |

**Key insight:** Phase 47 should orchestrate existing viewer capabilities, not recreate them in C#.

## Common Pitfalls

### Pitfall 1: Treating launch intent as import truth
**What goes wrong:** Host shows “JSONL loaded” when it only passed `--jsonl`.
**Why it happens:** mdCAD’s actual startup import state lives inside `src/app.c` and the startup controller, not in host-readable signals.
**How to avoid:** Use labels like `example resolved`, `jsonl requested`, `live refresh requested`, and point developers to the viewer overlay for runtime truth.
**Warning signs:** Status text claims success even when the example file is missing or mdCAD later shows an overlay error.

### Pitfall 2: Relaunching on a dead placeholder
**What goes wrong:** Second launch never attaches or attaches unpredictably.
**Why it happens:** Current teardown destroys the placeholder HWND as part of invalid-parent signaling.
**How to avoid:** Recreate the hosting control/placeholder before relaunch.
**Warning signs:** `_placeholderHwnd == IntPtr.Zero`, child attach timeout on second launch, or stale `GetParent(...)` failures.

### Pitfall 3: Leaving stale process/event state behind
**What goes wrong:** Orphaned `mdCAD.exe`, duplicated event callbacks, or misleading status after relaunch.
**Why it happens:** Reusing `_mdcadProcess`, timers, and cached failure text across sessions.
**How to avoid:** Centralize session reset; clear timers, event subscriptions, cached status lines, process references, and child HWND handles before each new launch.
**Warning signs:** Multiple `mdCAD.exe` processes, duplicate stderr lines, or status from the prior run leaking into the next run.

### Pitfall 4: Using repo-relative resource paths
**What goes wrong:** Running the host outside the repo root fails to find the example JSONL.
**Why it happens:** `Directory.GetCurrentDirectory()` is not a deployment contract.
**How to avoid:** Resolve from `AppContext.BaseDirectory` and copy content beside the built host.
**Warning signs:** Works only from the repo root or only under `dotnet run`.

## Code Examples

### Launch mdCAD with bundled example + optional live refresh
```csharp
// Source basis: samples/avalonia-host/MainWindow.axaml.cs:307-315
var args = new List<string>
{
    "--embedded",
    "--parent-hwnd", $"0x{_launchParentHwnd.ToInt64():X}",
    "--jsonl", exampleAbsolutePath
};
if (liveRefreshEnabled)
{
    args.Add("--jsonl-live-refresh");
}

startInfo.Arguments = string.Join(" ", args.Select(QuoteIfNeeded));
```

### Keep host status aligned to observable signals
```csharp
// Source basis: MainWindow.axaml.cs attach/teardown flow + src/app.c startup overlay ownership
LaunchStatus = "launching";
JsonlStatus = File.Exists(exampleAbsolutePath) ? "example resolved; --jsonl requested" : "example missing";
LiveRefreshStatus = liveRefreshEnabled ? "requested on" : "off";
AttachStatus = "waiting for child attach";
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Sample host as one-shot attach scaffold | Sample host as conformance harness for bundled launch + relaunch lifecycle | Phase 47 target | Adds proof value without changing the no-IPC architecture. |
| Temporary MinGW validation workaround | Prefer MSVC Vulkan `build-vulkan` path again | Post-Phase 46 / current user direction | Phase 47 validation should use `build-vulkan` for native build/test recommendations. |

**Deprecated/outdated:**
- The Phase 46 “use MinGW if VS is broken” operational note in `.planning/STATE.md` is stale for this phase. Future validation should prefer the working MSVC Vulkan `build-vulkan` path.

## Open Questions

1. **Should live refresh be a checkbox or a second launch button?**
   - What we know: Phase 46 already provides the CLI flag; Phase 47 needs a minimal proof UI.
   - What's unclear: preferred minimal host chrome shape.
   - Recommendation: Use a single checkbox/toggle plus one launch button; it keeps the host workflow-focused and avoids duplicate launch controls.

2. **How much host-side automation is worth adding?**
   - What we know: native contracts are already well-covered by CTest, but real HWND lifecycle proof is manual.
   - What's unclear: whether the phase should add a dedicated .NET test project for pure helper logic.
   - Recommendation: Do not add UI automation; keep automated coverage to existing native tests plus host build smoke, and capture the rest in a focused manual checklist.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| `dotnet` | Avalonia host build/run | ✓ | 10.0.300 SDK | — |
| `cmake` | Native build | ✓ | 4.3.2 | — |
| `ctest` | Native regression suite | ✓ | 4.3.2 | — |
| `build-vulkan` tree | Native executable + CTests | ✓ | existing configured tree | Reconfigure only if it becomes stale |
| `samples/avalonia-host/resources/examples` | HOST-02 bundled example | ✗ | — | Must be added in Phase 47 |

**Missing dependencies with no fallback:**
- `samples/avalonia-host/resources/examples` content does not exist yet; HOST-02 cannot pass until Phase 47 adds it.

**Missing dependencies with fallback:**
- None. Use the MSVC Vulkan `build-vulkan` path directly.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Existing CTest native suite + `dotnet build` host smoke + manual Windows host checklist |
| Config file | `src/CMakeLists.txt`, `samples/avalonia-host/AvaloniaHost.csproj` |
| Quick run command | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release && ctest --test-dir build-vulkan -C Release -R "embed_launch_config_test|startup_jsonl_import_controller_test|startup_jsonl_app_contract_test" --output-on-failure` |
| Full suite command | `cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure && dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` |

**Phase 47 note:** prefer the MSVC Vulkan `build-vulkan` path for all native validation commands.

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| HOST-02 | Bundled example exists, resolves to absolute path, and is passed through the existing launch contract | build smoke + manual | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` plus native `ctest --test-dir build-vulkan -C Release -R "embed_launch_config_test|startup_jsonl_import_controller_test" --output-on-failure` | ✅ build / ❌ host checklist |
| HOST-03 | Host exposes launch/attach/JSONL/live-refresh text without IPC | manual + source review | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` | ❌ Wave 0 checklist |
| HOST-04 | Repeated launch/resize/focus/close cycles do not leave orphaned processes | manual lifecycle smoke | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` plus `ctest --test-dir build-vulkan -C Release -R "embed_launch_config_test|embed_input_state_test" --output-on-failure` | ❌ Wave 0 checklist |

### Sampling Rate
- **Per task commit:** `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
- **Per wave merge:** `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release && ctest --test-dir build-vulkan -C Release -R "embed_launch_config_test|startup_jsonl_import_controller_test|startup_jsonl_app_contract_test|embed_input_state_test|embed_layout_state_test" --output-on-failure`
- **Phase gate:** Full suite green on `build-vulkan` plus a completed manual host lifecycle checklist before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] Phase 47 manual checklist artifact for: bundled example resolution, launch with/without live refresh, repeated launch/close/relaunch, resize/focus regression, and orphan-process inspection.
- [ ] Host reset seam identified in code review before implementation begins (placeholder recreation after teardown).
- [ ] Optional: cheap static/source-contract check for copied example content and `--jsonl` argument wiring if the planner wants more than build smoke.

## Sources

### Primary (HIGH confidence)
- `samples/avalonia-host/MainWindow.axaml` — current host UI surface/status controls
- `samples/avalonia-host/MainWindow.axaml.cs` — launch, attach, resize, teardown, and current gaps
- `samples/avalonia-host/AvaloniaHost.csproj` — host target/runtime/package baseline
- `src/app_launch_config.h` — existing CLI contract for `--embedded`, `--parent-hwnd`, `--jsonl`, `--jsonl-live-refresh`
- `src/app.c` — startup JSONL overlay ownership and embedded runtime behavior
- `src/startup_jsonl_import_controller.h` — startup JSONL import state machine
- `src/tests/embed_launch_config_test.c` — parser coverage
- `src/tests/startup_jsonl_import_controller_test.c` — startup controller coverage
- `.planning/ROADMAP.md`, `.planning/REQUIREMENTS.md`, `.planning/STATE.md` — phase contract and current milestone state
- Executed commands on 2026-05-15:
  - `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` ✅
  - `ctest --test-dir build-vulkan -C Release -R "embed_launch_config_test|startup_jsonl_import_controller_test|startup_jsonl_app_contract_test" --output-on-failure` ✅
  - `ctest --test-dir build-vulkan -C Release --output-on-failure` ✅ (24/24)

### Secondary (MEDIUM confidence)
- `.planning/phases/43-embed-contract-child-window-bootstrap/43-VERIFICATION.md`
- `.planning/phases/44-embedded-resize-focus-viewer-layout/44-VERIFICATION.md`
- `.planning/phases/44-embedded-resize-focus-viewer-layout/44-HUMAN-UAT.md`
- `.planning/phases/45-startup-jsonl-auto-import/45-VALIDATION.md`
- `.planning/phases/46-launch-time-live-refresh/46-VALIDATION.md`

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - based on repo configuration, resolved package assets, and successful local builds/tests
- Architecture: HIGH - based on direct inspection of the current host/native lifecycle seams
- Pitfalls: HIGH - based on already-proven Phase 43-46 lifecycle constraints and the current host code shape

**Research date:** 2026-05-15  
**Valid until:** 2026-06-14
