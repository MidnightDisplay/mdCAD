---
phase: 37
slug: anchor-scoped-flat-ingest
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-27
---

# Phase 37 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | ctest (C/CMake native tests) |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Quick run command** | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_import_ui_contract_test\|jsonl_flat_import_options_contract_test\|jsonl_flat_anchor_scoped_ingest_test" --output-on-failure` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` |

---

## Sampling Rate

- **After every task commit:** Run targeted flat JSONL tests including the new anchor-ingest contract.
- **After every plan wave:** Run the full phase target set.
- **Before `/gsd-verify-work`:** Full suite must be green.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 37-01-01 | 01 | 1 | FIMP-03 | integration | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_anchor_scoped_ingest_test" --output-on-failure` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/tests/jsonl_flat_anchor_scoped_ingest_test.c` — contract coverage for root/entry hierarchy, empty-entry filtering, and rerun naming collisions.
- [ ] `src/CMakeLists.txt` wiring — target and `add_test(...)` entry for `jsonl_flat_anchor_scoped_ingest_test`.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Hierarchy readability for large runs | FIMP-03 | Visual hierarchy ergonomics | Import a multi-entry JSONL and confirm root -> entry -> geometry structure with no empty entry anchors. |
| Re-import naming clarity | FIMP-03 | Label clarity is UX-sensitive | Re-import the same source repeatedly and verify deterministic suffix progression (`name`, `name (2)`, `name (3)`). |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity maintained through implementation
- [ ] Wave 0 closes all missing contract references
- [ ] `nyquist_compliant: true` set in frontmatter when complete

**Approval:** pending
