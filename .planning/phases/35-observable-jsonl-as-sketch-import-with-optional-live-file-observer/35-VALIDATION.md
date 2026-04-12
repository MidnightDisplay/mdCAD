---
phase: 35
slug: observable-jsonl-as-sketch-import-with-optional-live-file-observer
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-12
---

# Phase 35 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + native C test executables |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Quick run command** | `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests|scene_solver_contract" --output-on-failure` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated runtime** | ~120 seconds |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests|scene_solver_contract" --output-on-failure`
- **After every plan wave:** Run `ctest --test-dir build-vulkan -C Release --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 120 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 35-01-01 | 01 | 1 | P35-01 | integration | `ctest --test-dir build-vulkan -C Release -R "jsonl_sketch_import_test" --output-on-failure` | ❌ W0 | ⬜ pending |
| 35-01-02 | 01 | 1 | P35-02 | unit/integration | `ctest --test-dir build-vulkan -C Release -R "jsonl_sketch_mapping_test" --output-on-failure` | ❌ W0 | ⬜ pending |
| 35-02-01 | 02 | 1 | P35-03 | unit | `ctest --test-dir build-vulkan -C Release -R "jsonl_observer_state_test" --output-on-failure` | ❌ W0 | ⬜ pending |
| 35-02-02 | 02 | 1 | P35-04 | integration | `ctest --test-dir build-vulkan -C Release -R "jsonl_reparse_transaction_test" --output-on-failure` | ❌ W0 | ⬜ pending |
| 35-03-01 | 03 | 2 | P35-05 | UI/manual + smoke | `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests" --output-on-failure` | ✅ partial | ⬜ pending |
| 35-03-02 | 03 | 2 | P35-06 | unit | `ctest --test-dir build-vulkan -C Release -R "jsonl_label_contract_test" --output-on-failure` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/tests/jsonl_sketch_import_test.c` — verify new menu/import creates sketch (not root dump)
- [ ] `src/tests/jsonl_sketch_mapping_test.c` — type mapping, flattening, mesh ignore, unconstrained output
- [ ] `src/tests/jsonl_observer_state_test.c` — observer defaults, persistence, retry/auto-disable semantics
- [ ] `src/tests/jsonl_reparse_transaction_test.c` — authoritative replacement + failure keeps last good state
- [ ] `src/tests/jsonl_label_contract_test.c` — filename/path label semantics and relink updates
- [ ] `src/CMakeLists.txt` — add tests and `add_test(...)` entries

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Observer controls relocate from Entity Inspector to Active Sketch Workspace when sketch is active | P35-05 | Window/state-driven UX layout behavior | Import linked sketch, verify controls visible in inspector when inactive; activate sketch, verify controls move to workspace top and editable duplicate is hidden |
| Script overwrite warning is visible while JSONL link is active | P35-05 | Warning UX text placement | Open Script Editor for linked sketch with observer enabled; verify warning text appears and references overwrite-on-reparse behavior |
| Two-line observer message area shows latest two messages | P35-03 | UI presentation sequencing | Trigger two+ observer events (e.g., lock warning then parse warning) and verify only the two most recent are shown |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 120s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
