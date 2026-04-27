---
phase: 38
slug: observable-link-manual-transactional-refresh
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-27
---

# Phase 38 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | ctest (C/CMake native tests) |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Quick run command** | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_manual_refresh_test\|jsonl_flat_import_ui_contract_test\|jsonl_flat_import_options_contract_test\|jsonl_flat_anchor_scoped_ingest_test" --output-on-failure` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` |

---

## Sampling Rate

- **After every task commit:** Run the quick JSONL flat-observer slice.
- **After every plan wave:** Run the full phase target set for observer + flat ingest contracts.
- **Before `/gsd-verify-work`:** Full suite must be green.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 38-01-01 | 01 | 1 | OBSF-02, OBSF-03, OBSF-05, PERF-02 | integration | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_manual_refresh_test" --output-on-failure` | ❌ W0 | ⬜ pending |
| 38-01-02 | 01 | 1 | OBSF-01 | contract | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_import_ui_contract_test\|jsonl_flat_import_options_contract_test" --output-on-failure` | ✅ | ⬜ pending |
| 38-02-01 | 02 | 2 | OBSF-02, OBSF-03, OBSF-05 | integration | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_manual_refresh_test\|jsonl_reparse_transaction_test" --output-on-failure` | ❌/✅ | ⬜ pending |
| 38-02-02 | 02 | 2 | PERF-02 | integration | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_manual_refresh_test\|jsonl_flat_anchor_scoped_ingest_test" --output-on-failure` | ❌/✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/tests/jsonl_flat_observer_manual_refresh_test.c` — manual refresh transactional coverage for same-root commit/rollback and subtree replacement behavior.
- [ ] `src/tests/jsonl_flat_observer_inspector_contract_test.c` (or equivalent extension) — Inspector control surface contract for flat-root observer UX.
- [ ] `src/CMakeLists.txt` wiring — target and `add_test(...)` entries for new Phase 38 tests.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Inspector-only status clarity during background refresh | OBSF-03 | UX wording/readability | Trigger refresh from selected flat root and confirm running/success/failure state is visible in Inspector only. |
| Link-later workflow on previously unlinked flat root | OBSF-01, OBSF-02 | UI file-selection flow | Import flat root with link OFF, then link from Inspector and verify source path + label metadata update. |
| Selection fallback to root when selected child is replaced | OBSF-05 | Interaction coherence | Select a child under root, refresh successfully, and verify selection lands on root anchor. |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity maintained through implementation
- [ ] Wave 0 closes all missing contract references
- [ ] `nyquist_compliant: true` set in frontmatter when complete

**Approval:** pending
