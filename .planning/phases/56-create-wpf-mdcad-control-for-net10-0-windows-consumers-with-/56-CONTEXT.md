# Phase 56: Create WPF mdCAD control for net10.0-windows consumers with Avalonia-parity parameters plus full and minimal sample test projects - Context

**Gathered:** 2026-05-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 56 adds a WPF-hosted reusable mdCAD control that mirrors the current Avalonia control's host-facing contract for Windows consumers targeting `net10.0-windows`. The new WPF surface should preserve the same runtime and lifecycle truth as the Avalonia version: mdCAD stays an external Windows-only process attached through the existing child-HWND embedding seam, the control owns launch/relaunch/stop behavior, and consuming apps get both a richer in-repo diagnostic proof host and a smallest-possible minimal host sample.

In scope:
- A reusable WPF control/library project for Windows consumers targeting `net10.0-windows`
- WPF bindable properties and public methods that preserve Avalonia-parity startup and control semantics
- Reuse of the existing runtime bundle/output-copy contract so WPF consumers receive the committed `mdcad-runtime` payload automatically
- A full diagnostic WPF sample host that mirrors the existing Avalonia host proof surface
- A minimal sealed WPF sample host that mirrors the existing minimal Avalonia consumer path
- WPF-targeted tests/proof surfaces that lock parameter parity, launch snapshot behavior, and Windows embedding seams without widening runtime claims

Out of scope:
- Replacing or redesigning the existing Avalonia control
- Cross-platform WPF support or any non-Windows mdCAD runtime promise
- Host-to-viewer IPC, in-process embedding, or a new runtime command channel
- NuGet packaging/distribution work beyond local project-reference consumption
- Reworking native mdCAD embed semantics beyond what the WPF control needs to reuse

</domain>

<decisions>
## Implementation Decisions

### WPF control contract parity
- **D-01:** The WPF control should expose the same host-facing configuration surface as the Avalonia control wherever WPF idioms allow direct parity.
- **D-02:** The parameter/property set must include `JsonlPath`, `StartupLiveRefreshEnabled`, `ViewportOnlyStartupMode`, `AutoStart`, and `PresentationMode`.
- **D-03:** The public control API must keep explicit `StartAsync()` and `StopAsync()` entry points in addition to `AutoStart`.
- **D-04:** Launch-affecting setting changes after session start should remain control-managed relaunches using the newest snapshot rather than leaving restart orchestration to the host app.

### Windows runtime and embedding boundary
- **D-05:** The WPF control remains Windows-only at runtime and must continue using the existing child-HWND launch contract for mdCAD.
- **D-06:** The control must keep the committed runtime-bundle model and copy `mdcad-runtime/**` into the consumer output just like the Avalonia control.
- **D-07:** Unsupported-runtime messaging must stay truthful; this phase does not expand mdCAD runtime support beyond Windows.

### Sample proof surfaces
- **D-08:** The repository should include both a full diagnostic WPF sample host and a minimal sealed WPF sample host, mirroring the Avalonia proof split.
- **D-09:** The full WPF sample is the richer lifecycle/status harness; the minimal WPF sample is the smallest consumer/onboarding proof.
- **D-10:** WPF sample defaults and docs/test artifacts should align with the existing compile-time-vs-runtime truth established in the Avalonia milestone.

### The agent's discretion
- Choose the exact WPF hosting primitive (`HwndHost`, composition layout, helper classes) as long as it preserves the proven child-HWND attach/recreate/teardown behavior.
- Choose the exact project names/namespaces for the WPF control and WPF sample apps, as long as they remain consistent with repo conventions and clearly parallel the Avalonia artifacts.
- Choose the exact split between shared host-agnostic lifecycle code and WPF-specific shell code, as long as parity stays tight and duplication stays controlled.
- Choose the exact WPF test strategy and project layout, as long as the new phase locks the public contract and sample-proof surfaces credibly.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — active milestone context plus Phase 56 roadmap entry
- `.planning/REQUIREMENTS.md` — current host-compatibility/runtime-boundary requirements that Phase 56 must extend or map carefully
- `.planning/PROJECT.md` — current milestone goal and Windows-only embedding boundary
- `.planning/STATE.md` — recent roadmap evolution and pending follow-up order

### Closest prior phase artifacts
- `.planning/milestones/v1.8-phases/48-reusable-avalonia-mdcad-user-control/48-CONTEXT.md`
- `.planning/milestones/v1.8-phases/48-reusable-avalonia-mdcad-user-control/48-RESEARCH.md`

### Code anchors: Avalonia control parity source
- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs` — current public property/method surface and control-owned coordinator wiring
- `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj` — target framework and `mdcad-runtime` copy contract
- `samples/avalonia-mdcad-control/Host/` — backend, launch snapshot, session coordinator, and runtime resolver seams
- `samples/avalonia-mdcad-control.tests/` — current contract/backing tests to mirror or extend
- `samples/avalonia-host/` — full diagnostic consumer proof surface
- `samples/avalonia-host-minimal/` — smallest sealed consumer proof surface

</canonical_refs>

<code_context>
## Existing Code Insights

### Avalonia public contract to mirror
- `MdCadEmbeddedControl` currently exposes `JsonlPath`, `StartupLiveRefreshEnabled`, `ViewportOnlyStartupMode`, `AutoStart`, `PresentationMode`, `StartAsync()`, and `StopAsync()`.
- Launch snapshots are captured through `MdCadLaunchSnapshot.Create(JsonlPath, StartupLiveRefreshEnabled, ViewportOnlyStartupMode)`.
- The control already separates host shell concerns from backend/session coordination, which is the strongest reuse seam for a second UI stack.

### Packaging and proof patterns already established
- `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj` copies `runtime\win-x64\**\*` into `mdcad-runtime\` under consumer output with standard MSBuild content metadata.
- `samples/avalonia-host/` is the rich lifecycle harness and references the reusable control as a normal consumer project.
- `samples/avalonia-host-minimal/` proves the smallest reference/instantiate path against the same reusable control library.
- `samples/avalonia-mdcad-control.tests/` already contains useful parity targets for launch snapshots, session coordination, unsupported behavior, backend start-info generation, and runtime refresh integration.

### Integration direction for Phase 56
- Favor reusing shared lifecycle/runtime-resolution logic from the Avalonia control instead of cloning process-management behavior into a WPF-only fork.
- Keep WPF-specific work concentrated in the shell/placeholder surface and public dependency-property layer.
- Preserve the current runtime payload and CLI launch contract so both UI stacks remain thin hosts over the same embedded mdCAD runtime.

</code_context>

<specifics>
## Specific Ideas

- The user explicitly wants the WPF control to feel like the Avalonia control, with the same parameters and the same full-vs-minimal sample split.
- Consuming .NET projects are expected to target `net10.0-windows`.
- The likely closest WPF hosting primitive is `HwndHost`, but the phase may pick any equivalent Windows-native host surface that best preserves attach/recreate semantics.
- Phase planning should call out how WPF proof and test lanes stay separate from the existing Avalonia proof lanes while reusing as much shared control logic as practical.

</specifics>

<deferred>
## Deferred Ideas

- Broader docs/onboarding refresh unless the new WPF samples force immediate truthfulness updates
- New distribution/package strategy spanning both Avalonia and WPF controls
- Additional host-facing capabilities beyond strict parity with the existing Avalonia control
- Multi-control orchestration, richer diagnostics IPC, or cross-host abstraction cleanup beyond what Phase 56 needs

</deferred>

---

*Phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-*
*Context gathered: 2026-05-28*
