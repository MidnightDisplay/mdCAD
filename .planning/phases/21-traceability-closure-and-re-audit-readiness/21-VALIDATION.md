---
phase: 21
slug: traceability-closure-and-re-audit-readiness
status: draft
nyquist_compliant: true
wave_0_complete: true
created: 2026-04-07
---

# Phase 21 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + manual UAT evidence artifacts |
| **Config file** | `src/CMakeLists.txt` |
| **Quick run command** | `ctest -R "endpoint_pick|scene_solver_contract|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure` *(only if PH18-03 mismatch is detected)* |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` *(not default for this docs-first phase)* |
| **Estimated runtime** | quick <180s; full suite variable |

---

## Sampling Rate

- **After every task commit:** Run artifact parity checks (`REQUIREMENTS.md` ↔ `*-VERIFICATION.md` ↔ `*-SUMMARY.md`) and only run quick command when PH18-03 mismatch/ambiguity is detected.
- **After every plan wave:** Run phase-targeted closure checks (manual evidence + traceability matrix integrity); run quick command only if escalation trigger is hit.
- **Before `/gsd-verify-work`:** Required Phase 21 closure artifacts must be consistent and milestone re-audit output must show no residual target gaps.
- **Max feedback latency:** 180 seconds for conditional automated rerun path.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 21-04-01 | 04 | 4 | SKCH-01 | compile + contract wiring | `cmake --build build-vulkan --config Release --target mdCAD script_roundtrip_tests` | ✅ | ⬜ pending |
| 21-04-02 | 04 | 4 | SKCH-01 | targeted regression | `cmake --build build-vulkan --config Release --target mdCAD && ctest -R "script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ⬜ pending |
| 21-05-01 | 05 | 5 | SKCH-01, SKCH-02, SKCH-03 | manual rerun gate + build prep | `cmake --build build-vulkan --config Release --target mdCAD` | ✅ | ⬜ pending |
| 21-05-02 | 05 | 5 | SKCH-01, SKCH-02, SKCH-03 | authoritative docs parity | `python -c "from pathlib import Path; u=Path('.planning/phases/10-sketch-foundations-managers/10-HUMAN-UAT.md').read_text(encoding='utf-8'); v=Path('.planning/phases/10-sketch-foundations-managers/10-VERIFICATION.md').read_text(encoding='utf-8'); assert 'Dual-entrypoint sketch attachment' in u and 'GeometryManager multi-select undo UX' in u and 'solve status' in u and 'color policy' in u and 'geometry count' in u and 'constraint count' in u and '10-HUMAN-UAT.md' in v"` | ✅ | ⬜ pending |
| 21-05-03 | 05 | 5 | SKCH-01, SKCH-02, SKCH-03 | requirements traceability parity | `python -c "from pathlib import Path; t=Path('.planning/REQUIREMENTS.md').read_text(encoding='utf-8'); assert '| SKCH-01 | Phase 21 |' in t and '| SKCH-02 | Phase 21 |' in t and '| SKCH-03 | Phase 21 |' in t"` | ✅ | ⬜ pending |
| 21-06-01 | 06 | 6 | SKCH-01, SKCH-02, SKCH-03, PH18-03 | milestone re-audit parity | `python -c "from pathlib import Path; t=Path('.planning/v1.2-MILESTONE-AUDIT.md').read_text(encoding='utf-8'); assert all(x in t for x in ['SKCH-01','SKCH-02','SKCH-03','PH18-03'])"` | ✅ | ⬜ pending |
| 21-06-02 | 06 | 6 | SKCH-01, SKCH-02, SKCH-03, PH18-03 | closure matrix parity | `python -c "from pathlib import Path; t=Path('.planning/phases/21-traceability-closure-and-re-audit-readiness/21-VALIDATION.md').read_text(encoding='utf-8'); assert 'Phase 21 Closure Matrix' in t and all(x in t for x in ['SKCH-01','SKCH-02','SKCH-03','PH18-03'])"` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements:

- [x] Phase 10 manual checklist exists: `.planning/phases/10-sketch-foundations-managers/evidence/10-sketch-managers-smoke-checklist.md`
- [x] Phase 10 manual UAT record exists: `.planning/phases/10-sketch-foundations-managers/10-HUMAN-UAT.md`
- [x] Authoritative verification anchors exist: `10-VERIFICATION.md`, `18-VERIFICATION.md`
- [x] Milestone audit artifact exists: `.planning/v1.2-MILESTONE-AUDIT.md`
- [x] PH18 targeted regression tests already registered and runnable in CTest (`endpoint_pick`, `scene_solver_contract`, `script_roundtrip_tests`)

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Dual-entrypoint sketch attachment remains correct in runtime UI | SKCH-01 | Requires live UI interaction and visual confirmation | Execute Phase 10 smoke checklist SKCH-01 flow; log fresh result/timestamp in `10-HUMAN-UAT.md`; cite exact row in `10-VERIFICATION.md` |
| Sketch inspector surfaces and GeometryManager bulk undo workflow remain correct | SKCH-02, SKCH-03 | User-facing interaction and undo semantics require manual validation | Execute Phase 10 smoke checklist SKCH-02/SKCH-03 flows; log fresh result/timestamp in `10-HUMAN-UAT.md`; cite exact rows in `10-VERIFICATION.md` |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or explicit manual/Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without a verification mechanism
- [x] Wave 0 covers all required references
- [x] No watch-mode flags
- [x] Feedback latency < 180s for conditional automated rerun path
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending

---

## Phase 21 Closure Matrix

| Requirement ID | REQUIREMENTS.md Traceability Row | Authoritative Verification Artifact/Status | Summary/Frontmatter Parity | Final Disposition |
|---|---|---|---|---|
| SKCH-01 | `.planning/REQUIREMENTS.md` → `SKCH-01 \| Phase 21 \| Complete` | `.planning/phases/10-sketch-foundations-managers/10-VERIFICATION.md` frontmatter `status: passed`; Human Verification Results §1 = **Pass** (`.planning/phases/10-sketch-foundations-managers/10-HUMAN-UAT.md#1-dual-entrypoint-sketch-attachment`) | Implementation summaries (`10-01-SUMMARY.md`, `10-02-SUMMARY.md`) now align with authoritative passed closure promotion | **Closed (satisfied)** |
| SKCH-02 | `.planning/REQUIREMENTS.md` → `SKCH-02 \| Phase 21 \| Complete` | `.planning/phases/10-sketch-foundations-managers/10-VERIFICATION.md` frontmatter `status: passed`; Human Verification Results §2 = **Pass** | Claimed/completed in `10-02-SUMMARY.md`; phase-level and summary parity now aligned | **Closed (satisfied)** |
| SKCH-03 | `.planning/REQUIREMENTS.md` → `SKCH-03 \| Phase 21 \| Complete` | `.planning/phases/10-sketch-foundations-managers/10-VERIFICATION.md` Requirements Coverage marks satisfied and frontmatter is `status: passed` | Claimed/completed in `10-01-SUMMARY.md` and `10-03-SUMMARY.md`; no remaining parity mismatch | **Closed (satisfied)** |
| PH18-03 | `.planning/REQUIREMENTS.md` → `PH18-03 \| Phase 21 \| Complete` | `.planning/phases/18-add-undo-steps-for-endpoint-moves/18-VERIFICATION.md` frontmatter `status: passed`; Requirements Coverage marks `PH18-03` satisfied | `.planning/phases/18-add-undo-steps-for-endpoint-moves/18-03-SUMMARY.md` frontmatter now includes `requirements-completed: [PH18-03]` | **Closed (satisfied)** |

### Re-audit Completion Log (21-06)

- Milestone audit artifact refreshed in `.planning/v1.2-MILESTONE-AUDIT.md` with post-closure target disposition updates for SKCH-01/02/03 and PH18-03.
- Target requirement rows now align to authoritative closure evidence: SKCH-01/02/03 = `satisfied` (Phase 10 `status: passed`), PH18-03 = `satisfied` (Phase 18 `status: passed` + summary frontmatter parity).
- Milestone remains `status: gaps_found` truthfully due remaining non-target orphaned requirements and missing phase verification artifacts (13/14/17), not due Phase 21 target requirements.
