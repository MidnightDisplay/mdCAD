# Requirements: mdCAD

**Defined:** 2026-04-10
**Core Value:** Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.

## v1.5 Requirements

Requirements for the solver workflow robustness and script re-apply integrity milestone.

### Solver Convergence Robustness

- [ ] **SROB-01**: User can perform large-jump edits in quarter-arc closed-loop line/arc arrangements without requiring manual "wiggle to latch" behavior.
- [ ] **SROB-02**: User can apply large length changes in coupled line/arc loops and either get a solved update for all dependent participants or a transactional rollback with no partial corruption.
- [ ] **SROB-03**: User can continue editing immediately after an infeasible large-jump attempt without solver deadlock or stale failure lock.

### Deterministic Diagnostics and Solve Policy

- [ ] **DIAG-01**: User gets deterministic solve outcomes and deterministic failure-class diagnostics for identical operation sequences.
- [ ] **DIAG-02**: User gets actionable diagnostics for large-jump and mixed-constraint failures without misleading participant-type errors.
- [ ] **DIAG-03**: Developer can verify deterministic baseline + immediate rerun parity on the v1.5 targeted regression gate.

### Explicit Coincidence Authoring Semantics

- [ ] **COIN-01**: User authoring ArcAxisLine relations receives explicit coincidence constraints for required center/axis anchoring instead of relying on implicit coupling.
- [ ] **COIN-02**: User authoring line-end/arc-end tangency receives explicit endpoint coincidence semantics that remain stable under subsequent edits.

### PARALLEL / ALONG Parity

- [ ] **PARI-01**: User gets equivalent interaction behavior for geometrically equivalent setups authored with PARALLEL constraints versus ALONG-axis constraints.
- [ ] **PARI-02**: User can drag equivalent vertices in mirrored/linked PARALLEL and ALONG arrangements with consistent feasibility outcomes.

### Script Re-apply Integrity

- [x] **SCRI-01**: User re-applying scripts preserves constraint participant semantics (including role/sub-index intent) and does not regress into unsupported-participant solver failures for valid authored cases. — Validated in Phase 31: script-reapply-fidelity-foundation
- [x] **SCRI-02**: User re-applying scripts preserves entity color metadata instead of resetting geometry to default white. — Validated in Phase 31: script-reapply-fidelity-foundation
- [x] **SCRI-03**: User can re-apply the same script repeatedly and observe stable, deterministic geometry/constraint outcomes. — Validated in Phase 31: script-reapply-fidelity-foundation

## v1.6+ Requirements (Deferred)

### Solver Capability Expansion

- **SCAP-01**: mdCAD supports broader automatic constraint repair suggestions with explicit user-approved application.
- **SCAP-02**: mdCAD supports runtime-selectable advanced solver strategy profiles for different sketch scales.

### Script Workflow Enhancements

- **SIO-01**: Script re-apply produces a user-visible semantic diff report for remap and metadata preservation outcomes.
- **SIO-02**: Script workflows support richer metadata roundtrip beyond core geometry and color.

## Out of Scope

Explicitly excluded from v1.5 to prevent scope creep.

| Feature | Reason |
|---------|--------|
| New broad constraint families unrelated to reported robustness/reapply failures | v1.5 is a stabilization and integrity milestone, not feature expansion |
| External solver framework replacement | Adds high integration risk and violates current C-first low-friction approach |
| Non-deterministic retry/jitter-based convergence hacks | Undermines reproducibility and closure confidence |
| Major UI overhaul unrelated to solver/reapply integrity | Would dilute focus from high-priority workflow failures |

## Traceability

Which phases cover which requirements. Populated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| SROB-01 | Phase 33 | Pending |
| SROB-02 | Phase 33 | Pending |
| SROB-03 | Phase 33 | Pending |
| DIAG-01 | Phase 33 | Pending |
| DIAG-02 | Phase 33 | Pending |
| DIAG-03 | Phase 34 | Pending |
| COIN-01 | Phase 32 | Pending |
| COIN-02 | Phase 32 | Pending |
| PARI-01 | Phase 33 | Pending |
| PARI-02 | Phase 33 | Pending |
| SCRI-01 | Phase 31 | Complete |
| SCRI-02 | Phase 31 | Complete |
| SCRI-03 | Phase 31 | Complete |

**Coverage:**
- v1.5 requirements: 13 total
- Mapped to phases: 13
- Unmapped: 0 ✓

---
*Requirements defined: 2026-04-10*
*Last updated: 2026-04-10 after v1.5 roadmap creation*
