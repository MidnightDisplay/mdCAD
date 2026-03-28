# Requirements: mdCAD

**Defined:** 2026-03-26
**Core Value:** Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.

## v1.1 Requirements

Requirements for the long-tail migration milestone. Each maps to Phase 6+ roadmap work.

### Long-Tail Migration

- [x] **TAIL-01**: User can save and reload scene data through cglm-backed serializer math paths without reintroducing migrated `math3d` helper dependencies.
- [x] **TAIL-02**: User can import JSONL/PLY geometry through cglm-backed importer math paths without reintroducing migrated `math3d` helper dependencies.
- [x] **TAIL-03**: User can perform undo/redo and editor utility transform workflows with cglm-backed math and behavior parity to the pre-migration user experience.

### Thin Entrypoint Reduction

- [x] **TRED-01**: Runtime-critical migrated paths no longer rely on removable temporary thin-entrypoint migration glue.
- [ ] **TRED-02**: Remaining thin-entrypoint surface is intentionally minimal, documented, and aligned to long-term project-owned boundaries.

### Validation & Performance

- [x] **VAL-01**: Compare harness coverage includes long-tail migration touchpoints and passes strict checks on required parity cases.
- [ ] **VAL-02**: Expanded migrated math slice shows no native performance regression on macOS Metal and Windows Vulkan benchmark gates.
- [ ] **VAL-03**: Manual smoke workflows covering serializer/import/undo/editor interactions pass on native macOS and Windows validation paths.

## v1.2+ Requirements (Deferred)

### Platform Expansion

- **PLAT-01**: mdCAD validates the migrated math foundation on iOS native builds.
- **PLAT-02**: mdCAD validates the migrated math foundation on the web build and resolves any Web/WASM-specific math or alignment issues.

## Out of Scope

Explicitly excluded from v1.1 to prevent scope creep.

| Feature | Reason |
|---------|--------|
| Full iOS migration/validation closure | Deferred to keep v1.1 focused on long-tail migration debt retirement |
| Full web/WASM migration/validation closure | Deferred to keep v1.1 focused on long-tail migration debt retirement |
| Repo-wide one-shot `math3d` removal | Too risky; staged migration remains required |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| TAIL-01 | Phase 6 | Complete |
| TAIL-02 | Phase 7 | Complete |
| TAIL-03 | Phase 8 | Complete |
| TRED-01 | Phase 8 | Complete |
| TRED-02 | Phase 9 | Pending |
| VAL-01 | Phase 9 | Complete |
| VAL-02 | Phase 9 | Pending |
| VAL-03 | Phase 9 | Pending |

**Coverage:**
- v1.1 requirements: 8 total
- Mapped to phases: 8
- Unmapped: 0 ✓

---
*Requirements defined: 2026-03-26*
*Last updated: 2026-03-26 after v1.1 requirements approval*

