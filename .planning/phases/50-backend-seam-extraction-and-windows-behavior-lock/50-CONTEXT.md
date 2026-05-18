# Phase 50: Backend Seam Extraction and Windows Behavior Lock - Context

**Gathered:** 2026-05-15
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 50 isolates the existing Windows-only placeholder, attach, resize, runtime lookup, and process-launch logic behind an internal backend seam **without changing the shipped Windows runtime behavior**. This is a behavior-preserving refactor phase that prepares the control for later unsupported-platform handling and plain-`net10.0` host compatibility phases.

In scope:
- Extract an internal backend boundary around the current Win32 placeholder/runtime/process lifecycle code.
- Keep the existing public control surface stable while the internals are reorganized.
- Preserve the existing Windows child-HWND launch, runtime bundle lookup, attach/relaunch lifecycle, and sealed/diagnostic behavior.
- Keep the current Windows diagnostic harness authoritative for proof that behavior did not regress.

Out of scope:
- Plain `net10.0` TFM widening (Phase 52).
- Explicit unsupported-platform placeholder/no-launch behavior (Phase 51).
- New host-facing API surface beyond what already exists.
- Packaging redesign, IPC, or any broad embedding architecture change.

</domain>

<decisions>
## Implementation Decisions

### Phase boundary strictness
- **D-01:** Phase 50 stays a behavior-preserving refactor. It must not widen TFMs, land non-Windows placeholder behavior, or add new user-facing capabilities early.
- **D-02:** The current public control API stays unchanged in this phase; the work is about internal ownership and seam extraction only.

### Backend seam shape
- **D-03:** Keep one public control project/assembly and extract an **internal** backend seam rather than splitting the public consumer surface or package layout in Phase 50.
- **D-04:** The backend seam should make Windows runtime ownership more explicit, not less; the goal is to isolate Win32/runtime logic from the shared control shell while keeping the runtime contract library-owned.

### Windows behavior lock
- **D-05:** Phase 50 must explicitly preserve the current Windows runtime bundle lookup and `--embedded --parent-hwnd` process-launch path.
- **D-06:** Phase 50 done bar includes Windows proof that the diagnostic harness still launches, attaches, stops, and relaunches through the existing child-HWND workflow.
- **D-07:** Sealed and diagnostic surfaces must remain functionally equivalent during the extraction; any user-visible behavior change belongs in later phases.

### the agent's Discretion
- Choose the exact internal backend interface/class names and file layout, provided D-01 through D-07 remain true.
- Choose whether `MdCadLaunchSnapshot` stays as-is or gets split into a more backend-friendly shared request + Windows-specific launch shape, provided the public control API stays unchanged in this phase.
- Choose the exact test split between backend-neutral and Windows-specific proof, provided the Windows runtime seam stays explicitly verified.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — Phase 50 scope, dependencies, and success criteria.
- `.planning/REQUIREMENTS.md` — `WPRS-01` and `WPRS-02`, plus the later phases this extraction must prepare without absorbing.
- `.planning/PROJECT.md` — v1.9 goal and the locked compile-time-vs-runtime milestone framing.
- `.planning/STATE.md` — current milestone position and continuity notes.

### Milestone research
- `.planning/research/SUMMARY.md` — synthesized v1.9 research guidance and rollout order.
- `.planning/research/STACK.md` — recommended `net10.0` host-facing control contract and stack constraints.
- `.planning/research/ARCHITECTURE.md` — recommended shared shell + Windows backend + unsupported backend shape.
- `.planning/research/PITFALLS.md` — refactor traps and Windows regression warnings to preserve during extraction.
- `.planning/research/FEATURES.md` — compile-time vs runtime contract expectations and proof/doc obligations.

### Upstream embedding baseline
- `.planning/milestones/v1.8-phases/48-reusable-avalonia-mdcad-user-control/48-CONTEXT.md` — locked reusable-control boundary, sealed/diagnostic split, and runtime-bundle ownership decisions.
- `.planning/milestones/v1.8-phases/47-sample-host-workflow-proof/47-01-SUMMARY.md` — bundled example/resource and host-proof baseline.
- `.planning/milestones/v1.8-phases/47-sample-host-workflow-proof/47-02-SUMMARY.md` — relaunch/cleanup/status behavior the diagnostic harness already proved.

### Code anchors
- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs` — current shared shell + Windows runtime ownership mixed in one file.
- `samples/avalonia-mdcad-control/Host/EmbedNativeControlHost.cs` — current Win32 placeholder creation and child-window helpers.
- `samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs` — current launch/relaunch orchestration seam.
- `samples/avalonia-mdcad-control/Host/MdCadRuntimeResolver.cs` — current `mdcad-runtime` lookup contract.
- `samples/avalonia-host/MainWindow.axaml.cs` — existing Windows diagnostic harness lifecycle proof surface.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `MdCadSessionCoordinator` already owns coalesced start/stop/recreate orchestration and is the natural place to keep backend-neutral lifecycle control.
- `EmbedNativeControlHost` already contains the current Win32 placeholder/child-window helpers that should move behind the Windows backend boundary rather than stay in the shared shell.
- `MdCadRuntimeResolver` already centralizes `AppContext.BaseDirectory\\mdcad-runtime` lookup and should remain the authoritative Windows runtime resolution seam.
- `samples/avalonia-host/MainWindow.axaml.cs` already acts as the truthful Windows diagnostic harness for launch/attach/relaunch proof.

### Established Patterns
- The host side owns placeholder HWND creation, process launch, attach detection, resize sync, and cleanup; mdCAD owns rendering and scene/runtime inside the child window.
- The current control is sealed-by-default with diagnostic mode as explicit opt-in; this presentation split is already proven and should be preserved during extraction.
- Runtime packaging resolves only from the copied `mdcad-runtime` folder under consumer output; repo-root probing is already gone from the reusable-control path.

### Integration Points
- `MdCadEmbeddedControl` must become a thinner shared shell that delegates runtime-specific work instead of owning Win32 state directly.
- A new internal backend seam should encapsulate placeholder creation, runtime resolution, process launch, attach polling, and resize sync.
- The Windows diagnostic harness remains the main regression-proof integration point for this phase.

</code_context>

<specifics>
## Specific Ideas

- Use the recommended staged-refactor defaults from the Phase 50 discussion: no new product behavior in this phase, only internal seam extraction plus explicit Windows behavior lock.
- Keep one public control project/assembly; Phase 50 should not turn into a public packaging split or API redesign.
- Treat the Windows diagnostic harness as the truth source for “nothing regressed” rather than trying to invent a new proof surface during the refactor.

</specifics>

<deferred>
## Deferred Ideas

- Explicit unsupported-platform placeholder/no-launch behavior — Phase 51.
- Plain `net10.0` public TFM widening and consumer restore/build proof — Phase 52.
- Plain-host consumer proof plus Windows regression closure expansion — Phase 53.
- Docs/onboarding truthfulness cleanup — Phase 54.

</deferred>

---

*Phase: 50-backend-seam-extraction-and-windows-behavior-lock*
*Context gathered: 2026-05-15*
