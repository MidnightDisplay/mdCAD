---
phase: 45
slug: startup-jsonl-auto-import
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-15
---

# Phase 45 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + C executable tests (repo-native) |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Fast smoke command (<30s target)** | `ctest --test-dir build-vulkan -C Release -R "embed_launch_config_test\|startup_jsonl_import_controller_test\|startup_jsonl_app_contract_test" --output-on-failure` |
| **Quick run command** | `ctest --test-dir build-vulkan -C Release -R "embed_launch_config_test\|startup_jsonl_import_controller_test\|startup_jsonl_app_contract_test\|jsonl_flat_import_options_contract_test\|jsonl_flat_anchor_scoped_ingest_test" --output-on-failure` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated fast smoke runtime** | <30 seconds target |
| **Estimated quick/full runtime** | ~60 seconds / existing full-suite runtime |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build-vulkan -C Release -R "embed_launch_config_test\|startup_jsonl_import_controller_test\|startup_jsonl_app_contract_test" --output-on-failure`
- **After every plan wave:** Run `ctest --test-dir build-vulkan -C Release -R "embed_launch_config_test\|startup_jsonl_import_controller_test\|startup_jsonl_app_contract_test\|jsonl_flat_import_options_contract_test\|jsonl_flat_anchor_scoped_ingest_test" --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency (per-task smoke):** <30 seconds target
- **Wave/phase gate latency budget:** ~60 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 45-01-01 | 01 | 1 | JSON-01 | unit/contract | `ctest --test-dir build-vulkan -C Release -R "embed_launch_config_test\|startup_jsonl_import_controller_test" --output-on-failure` | ❌ Wave 0 | ⬜ pending |
| 45-01-02 | 01 | 1 | JSON-01, JSON-03 | unit/integration | `ctest --test-dir build-vulkan -C Release -R "embed_launch_config_test\|startup_jsonl_import_controller_test\|jsonl_flat_import_options_contract_test\|jsonl_flat_anchor_scoped_ingest_test" --output-on-failure` | ❌ Wave 0 | ⬜ pending |
| 45-02-01 | 02 | 2 | JSON-03 | source-contract | `ctest --test-dir build-vulkan -C Release -R "startup_jsonl_app_contract_test" --output-on-failure` | ❌ Wave 0 | ⬜ pending |
| 45-02-02 | 02 | 2 | JSON-01, JSON-03 | integration | `ctest --test-dir build-vulkan -C Release -R "startup_jsonl_app_contract_test\|startup_jsonl_import_controller_test\|embed_launch_config_test\|jsonl_flat_import_options_contract_test\|jsonl_flat_anchor_scoped_ingest_test" --output-on-failure` | ❌ Wave 0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `src/tests/embed_launch_config_test.c` — extend parser coverage for valid absolute `--jsonl`, duplicate flag rejection, missing value, and relative-path rejection.
- [x] `src/tests/startup_jsonl_import_controller_test.c` — add controller coverage for flat-import defaults, sync/async ticking, and runtime-failure latching.
- [x] `src/tests/startup_jsonl_app_contract_test.c` — add source-contract coverage that `app.c` owns startup import outside `ui_scene_hierarchy_draw()` / `state.ui_visible`.
- [x] Existing infrastructure covers framework/tooling; no new test framework installation required.

---

## Manual-Only Verifications

- Launch mdCAD with a known good absolute JSONL path and confirm the import begins without requiring the Scene Hierarchy panel.
- Launch mdCAD with a missing absolute JSONL path and confirm the viewer stays open while a clear startup error surface is shown.

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all planned new test references
- [x] No watch-mode flags
- [x] Per-task fast smoke feedback latency target < 30s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** complete
