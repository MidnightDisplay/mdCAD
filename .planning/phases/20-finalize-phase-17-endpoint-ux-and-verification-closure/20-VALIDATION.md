---
phase: 20
slug: finalize-phase-17-endpoint-ux-and-verification-closure
status: draft
nyquist_compliant: true
wave_0_complete: true
created: 2026-04-07
---

# Phase 20 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + native C test executables |
| **Config file** | `src/CMakeLists.txt` |
| **Quick run command** | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` |
| **Full suite command** | `ctest -R "scene_solver_contract|scene_solver_drag|endpoint_pick|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated runtime** | quick <60s, full targeted gate ~60-180s |

---

## Sampling Rate

- **After every task commit:** Run requirement-scoped targeted command (default quick command above).
- **After every plan wave:** Run full targeted gate command.
- **Before `/gsd-verify-work`:** Full targeted gate must be green.
- **Max feedback latency:** 180 seconds.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 20-01-01 | 01 | 1 | D-01, D-02, D-03, D-04, D-05, D-06, D-07, D-08 | contract/integration traceability | `ctest -R "scene_solver_contract|scene_solver_drag|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ⬜ pending |
| 20-02-01 | 02 | 1 | D-09, D-10, D-11, D-12 | endpoint integration + legality/layering | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ⬜ pending |
| 20-03-01 | 03 | 2 | D-01..D-12 closure gate | suite + artifact audit | `ctest -R "scene_solver_contract|scene_solver_drag|endpoint_pick|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements:

- [x] `scene_solver_contract` CTest target registered in `src/CMakeLists.txt`
- [x] `scene_solver_drag` CTest target registered in `src/CMakeLists.txt`
- [x] `scene_solver_diagnostics` CTest target registered in `src/CMakeLists.txt`
- [x] `endpoint_pick` CTest target registered in `src/CMakeLists.txt`
- [x] Existing `build-vulkan` CTest command path used by prior Phase 17 artifacts

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Endpoint visibility and first-class selection in normal viewport flow | D-09, D-10, D-12 | Final UX confirmation remains interaction- and render-layer behavior | Reuse `.planning/phases/17-constraint-driven-geometry-solving/17-UAT.md` Tests 1 and 6/7 with explicit row citations; rerun only if ambiguous |
| Point-context legality for endpoint selection | D-10, D-11 | Menu correctness is user-facing semantics | Reuse `17-UAT.md` Test 2 plus legality test anchors in `src/tests/endpoint_pick_test.c`; rerun manual only if citation ambiguity exists |
| Endpoint Coincident chain and loop authoring reliability | D-11 | Interaction reliability under overlapping geometry requires manual confidence check | Reuse `17-UAT.md` Test 3 and closure notes from `17-05-SUMMARY.md`; rerun only if row cannot be closed unambiguously |
| Unsatisfiable drag feedback clarity | D-04, D-05 | Visual feedback quality is manual-only | Reuse prior Phase 17 validation/UAT references and recheck only if evidence remains conditional |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 180s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
