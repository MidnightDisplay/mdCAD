# Phase 51: Unsupported-Platform Contract - Context

**Gathered:** 2026-05-18
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 51 makes non-Windows behavior intentional, visible, and safe for the reusable Avalonia control without changing the already-approved Windows embedding/runtime path. This phase delivers the unsupported-platform contract only: a truthful placeholder surface, deterministic no-launch behavior, and stable imperative API semantics when mdCAD runtime embedding is unavailable.

In scope:
- Explicit non-Windows placeholder and warning behavior for the reusable control
- Preventing `AutoStart` and launch-affecting property changes from attempting mdCAD launch on unsupported platforms
- Stable unsupported-platform behavior for `StartAsync()` and `StopAsync()`
- Preserving the sealed-by-default vs diagnostic-mode split while telling the truth on unsupported platforms

Out of scope:
- Public TFM widening to plain `net10.0` (Phase 52)
- Windows runtime refresh automation (Phase 52.1)
- Consumer proof / docs truthfulness expansion (Phases 53-54)
- Any change to the approved Windows child-HWND/runtime behavior proven in Phase 50

</domain>

<decisions>
## Implementation Decisions

### Unsupported placeholder presentation
- **D-01:** Unsupported platforms use the same core truthful message in both presentation modes.
- **D-02:** `sealed` stays minimal, while `diagnostic` adds extra status/detail around the same unsupported-platform truth.
- **D-03:** The core message must explain that the host/control is valid, but embedded mdCAD viewing is Windows-only and will not launch on the current platform.

### Imperative API contract
- **D-04:** On unsupported platforms, `StartAsync()` fails immediately with a clear unsupported-runtime error instead of silently succeeding or no-oping.
- **D-05:** The `StartAsync()` failure text should match the same truth shown in the placeholder/warning surface.
- **D-06:** On unsupported platforms, `StopAsync()` is a safe no-op and preserves the unsupported warning/state.

### Auto-start and launch-affecting settings
- **D-07:** Unsupported placeholder/state should appear as soon as the control attaches to the visual tree; `AutoStart` never attempts runtime launch there.
- **D-08:** Launch-affecting property changes (`JsonlPath`, live refresh, related launch intent) update descriptive state only on unsupported platforms; they do not trigger launch work.
- **D-09:** In `diagnostic` mode, requested JSONL/live-refresh intent may still be shown as informational-only state on unsupported platforms.

### Warning precedence and diagnostic detail
- **D-10:** Unsupported-platform messaging remains the primary warning/detail truth on unsupported hosts.
- **D-11:** Requested JSONL/live-refresh information may appear only as secondary informational diagnostics; normal Windows-oriented JSONL/path validation must not take precedence there.
- **D-12:** In `diagnostic` mode, the control's own launch action is disabled up front when runtime activation is impossible.

### the agent's Discretion
- Choose the exact internal unsupported-backend type/file names and how backend selection is wired, as long as the public control API stays stable and Windows behavior remains untouched.
- Choose the exact visual styling, spacing, and iconography of the unsupported placeholder, as long as `sealed` stays minimal and `diagnostic` remains more explicit.
- Choose the exact exception type used for unsupported `StartAsync()` failure, as long as the failure is immediate and the message matches the locked placeholder truth.
- Choose the exact wording/placement of secondary informational diagnostics for requested JSONL/live-refresh intent, as long as unsupported-platform messaging remains primary.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — Phase 51 goal, dependency on Phase 50, and success criteria for the unsupported-platform contract.
- `.planning/REQUIREMENTS.md` — `PLAT-01`, `PLAT-02`, and `PLAT-03`, which define the required placeholder, no-launch, and imperative API behavior.
- `.planning/PROJECT.md` — milestone goal that widens host compatibility while keeping embedded mdCAD runtime support Windows-only.
- `.planning/STATE.md` — current milestone continuity, next-step routing, and preserved Windows constraints from completed Phase 50.

### Milestone research
- `.planning/research/SUMMARY.md` — recommended rollout order and the need for an explicit unsupported backend before public TFM widening.
- `.planning/research/FEATURES.md` — supported feature expectations, desirable-but-optional capability ideas, and anti-features to avoid.
- `.planning/research/ARCHITECTURE.md` — shared shell + Windows backend + unsupported backend shape, plus warning-precedence guidance.
- `.planning/research/PITFALLS.md` — silent-idle and misleading JSONL-first warning traps to avoid on unsupported hosts.

### Upstream reusable-control baselines
- `.planning/milestones/v1.8-phases/48-reusable-avalonia-mdcad-user-control/48-CONTEXT.md` — sealed/diagnostic split, Windows-only runtime packaging, and property-driven relaunch expectations carried into v1.9.
- `.planning/phases/50-backend-seam-extraction-and-windows-behavior-lock/50-CONTEXT.md` — internal backend-seam boundary and preserved Windows behavior from the completed extraction phase.
- `.planning/phases/50-backend-seam-extraction-and-windows-behavior-lock/50-03-SUMMARY.md` — approved Windows diagnostic-host proof that must remain unchanged while adding unsupported-platform behavior.

### Code anchors
- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs` — shared shell, warning surface, diagnostic surface, backend construction, and public imperative API entrypoints.
- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml` — visual layout for sealed vs diagnostic presentation.
- `samples/avalonia-mdcad-control/Host/IMdCadEmbedBackend.cs` — internal backend seam the unsupported backend should implement.
- `samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs` — shared `AutoStart` / `StartAsync()` / `StopAsync()` orchestration that Phase 51 must keep deterministic.
- `samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs` — supported-platform reference behavior that Phase 51 must leave unchanged on Windows.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `MdCadEmbeddedControl` warning and diagnostic surfaces — already provide a host-owned place to show truthful unsupported-platform state without inventing a new public control shape.
- `IMdCadEmbedBackend` — already isolates runtime-specific behavior, so an unsupported backend can slot in without reopening the public API.
- `MdCadSessionCoordinator` — already centralizes `AutoStart`, explicit start/stop, and launch-affecting reconcile behavior that Phase 51 needs to keep deterministic.
- `WindowsMdCadEmbedBackend` — provides the supported-runtime reference path Phase 51 must preserve while adding the unsupported branch.

### Established Patterns
- `sealed` is the default/minimal presentation, while `diagnostic` is an explicit truth-telling proof surface with extra controls/status.
- Host-owned warning/detail text is the established way to communicate launch/runtime problems; there is still no IPC-driven machine-readable readiness channel.
- Launch-affecting inputs are captured through the control properties and coordinator snapshot flow; later phases widen TFM or docs, not this one.

### Integration Points
- Backend selection in `MdCadEmbeddedControl` is the natural hook for supported vs unsupported runtime behavior.
- Unsupported placeholder rendering will likely touch both `MdCadEmbeddedControl.axaml` and `MdCadEmbeddedControl.axaml.cs`.
- Deterministic unsupported behavior should connect through the existing coordinator and backend seam rather than inventing a second lifecycle path.

</code_context>

<specifics>
## Specific Ideas

- Keep one consistent core message across both modes: the host/control is valid, but embedded mdCAD viewing is Windows-only on the current platform.
- `sealed` should remain minimal instead of turning into a mini diagnostic panel; `diagnostic` can expose extra unsupported state and disabled launch affordances.
- Diagnostic mode may still surface requested JSONL/live-refresh intent, but only as informational context subordinate to the unsupported-platform message.

</specifics>

<deferred>
## Deferred Ideas

- Add a public `IsRuntimeSupported`-style capability property — useful, but not required for Phase 51 and could widen the surface unnecessarily.
- Support host-customizable unsupported placeholder text — valuable polish, but not part of the current contract phase.
- Public TFM widening to plain `net10.0` and the proof host restore/build closure — Phase 52.
- Windows runtime refresh automation via dotnet-managed helper flow — Phase 52.1.
- Consumer proof and docs/onboarding truthfulness expansion — Phases 53 and 54.

</deferred>

---

*Phase: 51-unsupported-platform-contract*
*Context gathered: 2026-05-18*
