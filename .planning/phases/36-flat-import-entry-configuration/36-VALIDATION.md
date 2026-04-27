---
phase: 36
slug: flat-import-entry-configuration
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-27
---

# Phase 36 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | ctest (C/CMake native tests) |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Quick run command** | `ctest --output-on-failure -R "jsonl_loader_limits_test|jsonl_sketch_import_test|jsonl_observer_state_test"` |
| **Full suite command** | `ctest --output-on-failure` |
| **Estimated runtime** | ~120 seconds |

---

## Sampling Rate

- **After every task commit:** Run `ctest --output-on-failure -R "jsonl_loader_limits_test|jsonl_sketch_import_test|jsonl_observer_state_test"`
- **After every plan wave:** Run `ctest --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 120 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 36-01-01 | 01 | 1 | FIMP-01 | integration | `ctest --output-on-failure -R "jsonl_flat_import_ui_contract_test"` | ❌ W0 | ⬜ pending |
| 36-01-02 | 01 | 1 | FIMP-02 | integration | `ctest --output-on-failure -R "jsonl_flat_import_options_contract_test"` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/tests/jsonl_flat_import_ui_contract_test.c` — verify dedicated flat-large menu/import entry contract for FIMP-01.
- [ ] `src/tests/jsonl_flat_import_options_contract_test.c` — verify popup option-to-import-start mapping for FIMP-02.
- [ ] `src/CMakeLists.txt` test target wiring — add both tests and `add_test(...)` entries.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| File menu discoverability and wording clarity of third action | FIMP-01 | UX text clarity is visual/contextual | Launch app, open File menu, confirm new action label and existing two JSONL actions remain visible and unchanged. |
| Popup control ordering/readability for mirrored options | FIMP-02 | Layout ergonomics are UI-sensitive | Open flat-large popup from selected JSONL and verify all option groups are present and understandable before import start. |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 120s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
