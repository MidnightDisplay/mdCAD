---
phase: 40
slug: large-dump-stability-interaction-coherence
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-27
---

# Phase 40 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + native C test executables |
| **Config file** | `src/CMakeLists.txt` |
| **Quick run command** | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_(manual_refresh_test|auto_safety_test|inspector_contract_test)" --output-on-failure` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated runtime** | ~30 seconds (targeted slice), full suite environment-dependent |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_(manual_refresh_test|auto_safety_test|inspector_contract_test)" --output-on-failure`
- **After every plan wave:** Run `ctest --test-dir build-vulkan -C Release --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 60 seconds (targeted slice on current workstation)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 40-01-01 | 01 | 1 | PERF-01 | integration | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_auto_safety_test" --output-on-failure` | ✅ | ⬜ pending |
| 40-01-02 | 01 | 1 | PERF-01 | integration | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_manual_refresh_test" --output-on-failure` | ✅ | ⬜ pending |
| 40-02-01 | 02 | 2 | PERF-03 | integration | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_(manual_refresh_test|auto_safety_test|inspector_contract_test)" --output-on-failure` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/tests/jsonl_flat_observer_auto_safety_test.c` — add burst coalescing assertions for one-active + one-pending rerun behavior
- [ ] `src/tests/jsonl_flat_observer_manual_refresh_test.c` — add repeated-cycle coherence assertions for root stability and selection remap
- [ ] `src/tests/jsonl_flat_observer_inspector_contract_test.c` — add/adjust contract assertions if inspector coherence status copy expands

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Very-large dataset stress run remains operational | PERF-01 | Dataset and machine variance are too high for deterministic CI thresholding | Import very-large JSONL, trigger repeated updates, verify no crash/hang, record advisory timing and observed responsiveness |
| Hierarchy/inspector usability during repeated large refresh | PERF-03 | ImGui interaction continuity is partly UX-observed | Keep root selected/visible, run repeated refresh cycles, verify inspector remains coherent and root remains stable interaction pivot |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s for quick slice
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
