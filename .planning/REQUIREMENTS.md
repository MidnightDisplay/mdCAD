# Requirements: mdCAD

**Defined:** 2026-05-05
**Core Value:** Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.

## v1.7 Requirements

### Flat Import Steady State

- [ ] **FIMP-05**: User can import a large flat JSONL file with `Link file for refresh (optional)` enabled and keep the full imported line/point geometry visible after the import settles.
- [ ] **FIMP-06**: User can rely on Scene Hierarchy totals for a linked large flat import converging to the committed final entity counts instead of continuing to drift after geometry first appears in the viewport.

### Anchor Observer Refresh

- [ ] **OBSF-07**: User can keep a linked large flat import under observer refresh without the visible geometry collapsing to a tail subset or joints-only remnants.
- [ ] **OBSF-08**: User can refresh a linked large flat import transactionally so replaced geometry is removed cleanly and retained geometry is not prematurely deleted during re-import.

### Large-File Reliability

- [ ] **PERF-04**: User can delete a linked flat import root after prior refresh activity and remove all related line and point instances from both the viewport and slot-buffer debug state.
- [ ] **PERF-05**: User can rely on large linked flat imports reaching a stable final visible state after import or refresh instead of continuing observer-related background churn that changes counts or rendered coverage late.

## v1.8+ Requirements (Deferred)

### Flat Import Enhancements

- **FIMP-04**: User can perform diff/patch-based incremental refresh instead of full subtree replacement.
- **OBSF-06**: User can configure advanced observer scheduling profiles per imported anchor.

## Out of Scope (v1.7)

| Feature | Reason |
|---------|--------|
| Broad flat-import UX expansion beyond the linked large-file regression | Keep the milestone focused on measurable bug closure |
| Incremental/diff-based refresh architecture changes | Correctness of the existing subtree replacement flow comes first |
| New parser or external watcher dependencies | The regression should be fixed within the current import/observer stack |
| Converting flat imports into sketches or adding constraint inference | Separate capability, outside this milestone's bug-fix scope |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| FIMP-05 | Phase 41 | Pending |
| FIMP-06 | Phase 41 | Pending |
| OBSF-07 | Phase 42 | Pending |
| OBSF-08 | Phase 42 | Pending |
| PERF-04 | Phase 42 | Pending |
| PERF-05 | Phase 42 | Pending |

**Coverage:**
- v1.7 requirements: 6 total
- Mapped to phases: 6
- Unmapped: 0 ✅

---
*Requirements defined: 2026-05-05*
*Last updated: 2026-05-05 after roadmap creation*
