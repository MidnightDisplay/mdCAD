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
| 21-01-01 | 01 | 1 | SKCH-01, SKCH-02, SKCH-03 | manual + artifact traceability | `cmake --build build-vulkan --config Release --target mdCAD` (manual prep); manual run against Phase 10 smoke checklist | ✅ | ⬜ pending |
| 21-02-01 | 02 | 2 | PH18-03 | docs parity + conditional regression | `ctest -R "endpoint_pick|scene_solver_contract|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure` *(only if mismatch appears)* | ✅ | ⬜ pending |
| 21-03-01 | 03 | 3 | SKCH-01, SKCH-02, SKCH-03, PH18-03 | audit + cross-file closure matrix | `node ".github/get-shit-done/bin/gsd-tools.cjs" audit milestone v1.2 --refresh` *(or repo-equivalent milestone audit refresh command used in prior phases)* | ✅ | ⬜ pending |

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
