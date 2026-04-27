# Requirements: mdCAD

**Defined:** 2026-04-27
**Core Value:** Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.

## v1 Requirements

Requirements for milestone **v1.6: Observable Flat JSONL Import for Large Geometry Dumps**.

### Flat Import UX

- [ ] **FIMP-01**: User can start flat JSONL import from a dedicated menu/import mode for large geometry dumps.
- [ ] **FIMP-02**: User can configure flat import options (scale, rotation, shift, and color behavior) before import.
- [ ] **FIMP-03**: Flat import creates a single root anchor and imports entries as plain non-sketch scene entities under that anchor.

### Anchor Observer Refresh

- [ ] **OBSF-01**: User can opt in or opt out of file observability during flat import, with default set to OFF.
- [ ] **OBSF-02**: Imported anchor stores source-link metadata and replay settings required for refresh.
- [ ] **OBSF-03**: User can trigger manual refresh to re-import source data into the same anchor.
- [ ] **OBSF-04**: Automatic observer refresh uses debounce/retry/auto-disable safety behavior for unstable file-write windows.
- [ ] **OBSF-05**: Refresh is transactional: commit on success, preserve last-good anchor content on failure.

### Large-File Reliability

- [ ] **PERF-01**: Large flat JSONL imports remain responsive and avoid UI lockups/crashes during import and refresh.
- [ ] **PERF-02**: Flat refresh path avoids sketch/script overhead and operates on anchor-subtree replacement semantics.
- [ ] **PERF-03**: Anchor selection, hierarchy visibility, and inspector interaction remain coherent across repeated refreshes.

## v2 Requirements

Deferred to future milestones.

### Flat Import Enhancements

- **FIMP-04**: User can perform diff/patch-based incremental refresh instead of full subtree replacement.
- **OBSF-06**: User can configure advanced observer scheduling profiles per imported anchor.

## Out of Scope

Explicit exclusions for v1.6.

| Feature | Reason |
|---------|--------|
| Auto-convert flat imports into sketch entities | Reintroduces sketch overhead and defeats large-dump performance goal |
| New parser or external watcher libraries | Unnecessary dependency churn for milestone scope |
| Constraint authoring/inference during flat import | Separate capability, outside flat observable import scope |

## Traceability

Will be populated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| FIMP-01 | TBD | Pending |
| FIMP-02 | TBD | Pending |
| FIMP-03 | TBD | Pending |
| OBSF-01 | TBD | Pending |
| OBSF-02 | TBD | Pending |
| OBSF-03 | TBD | Pending |
| OBSF-04 | TBD | Pending |
| OBSF-05 | TBD | Pending |
| PERF-01 | TBD | Pending |
| PERF-02 | TBD | Pending |
| PERF-03 | TBD | Pending |

**Coverage:**
- v1 requirements: 11 total
- Mapped to phases: 0
- Unmapped: 11 ⚠️

---
*Requirements defined: 2026-04-27*
*Last updated: 2026-04-27 after v1.6 milestone requirement definition*
