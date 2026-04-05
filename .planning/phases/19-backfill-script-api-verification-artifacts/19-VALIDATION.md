---
phase: 19
slug: backfill-script-api-verification-artifacts
status: draft
nyquist_compliant: true
wave_0_complete: true
created: 2026-04-05
---

# Phase 19 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CMake/CTest native targets (`script_roundtrip_tests`) |
| **Config file** | `src/CMakeLists.txt` |
| **Quick run command** | `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` |
| **Full suite command** | `cmake --build build-vulkan --config Release --target script_roundtrip_tests && ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` |
| **Estimated runtime** | smoke <60s, full targeted gate ~60-180s |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests`
- **After every plan wave:** Run `cmake --build build-vulkan --config Release --target script_roundtrip_tests && ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests`
- **Before `/gsd-verify-work`:** Full targeted gate must be green
- **Max feedback latency:** 180 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 19-01-01 | 01 | 1 | SCRP-01, SCRP-02, SCRP-03, SCRP-06 | integration + traceability | `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` | ✅ | ⬜ pending |
| 19-02-01 | 02 | 1 | SCRP-04, SCRP-05, API-01, API-02 | integration + transactional regression | `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements:

- [x] `script_roundtrip_tests` target registered in `src/CMakeLists.txt`
- [x] Script/API façade and undo transaction coverage exists in `src/tests/script_roundtrip_tests.c`
- [x] Windows MSVC + Vulkan targeted rerun command path is established

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Script Editor standalone UX acceptance parity | SCRP-01 | Prior acceptance is interactive/UI-behavioral | Reuse approved evidence from `.planning/phases/13-script-round-trip-baseline/13-03-SUMMARY.md` with explicit citation in verification doc |
| Script IO window live interaction parity | SCRP-05 | Prior acceptance is interactive/UI-behavioral | Reuse approved evidence from `.planning/phases/14-script-io-api-undo-integration/14-03-SUMMARY.md` with explicit citation in verification doc |
| One-apply-one-undo transactional user feel | API-02 | Final user-facing undo semantics include interaction feel | Reuse approved Phase 14 UAT/manual outcome citations and pair with fresh `script_roundtrip_tests` rerun evidence |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 180s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
