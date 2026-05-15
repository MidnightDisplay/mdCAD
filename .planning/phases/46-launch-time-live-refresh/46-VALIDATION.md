---
phase: 46
slug: launch-time-live-refresh
status: complete
nyquist_compliant: true
wave_0_complete: false
created: 2026-05-15
---

# Phase 46 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + standalone C test executables |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Fast smoke command (<30s target)** | `ctest --test-dir build-vulkan -C Release --output-on-failure -R "(embed_launch_config_test|startup_jsonl_import_controller_test|startup_jsonl_app_contract_test)"` |
| **Quick run command** | `ctest --test-dir build-vulkan -C Release --output-on-failure -R "(embed_launch_config_test|startup_jsonl_import_controller_test|startup_jsonl_app_contract_test|jsonl_flat_observer_manual_refresh_test|jsonl_flat_observer_auto_safety_test)"` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated fast smoke runtime** | <30 seconds target |
| **Estimated quick/full runtime** | ~60 seconds / existing full-suite runtime |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build-vulkan -C Release --output-on-failure -R "(embed_launch_config_test|startup_jsonl_import_controller_test|startup_jsonl_app_contract_test)"`
- **After every plan wave:** Run `ctest --test-dir build-vulkan -C Release --output-on-failure -R "(jsonl_flat_observer_manual_refresh_test|jsonl_flat_observer_auto_safety_test|embed_launch_config_test|startup_jsonl_import_controller_test|startup_jsonl_app_contract_test)"`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency (per-task smoke):** <30 seconds target
- **Wave/phase gate latency budget:** ~60 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 46-01-01 | 01 | 1 | JSON-02 | unit/contract | `ctest --test-dir build-vulkan -C Release --output-on-failure -R "(embed_launch_config_test|startup_jsonl_import_controller_test)"` | ❌ Wave 0 | ⬜ pending |
| 46-01-02 | 01 | 1 | JSON-02, JSON-04 | unit/integration | `ctest --test-dir build-vulkan -C Release --output-on-failure -R "(embed_launch_config_test|startup_jsonl_import_controller_test|jsonl_flat_observer_manual_refresh_test|jsonl_flat_observer_auto_safety_test)"` | ❌ Wave 0 | ⬜ pending |
| 46-02-01 | 02 | 2 | JSON-04 | source-contract | `ctest --test-dir build-vulkan -C Release --output-on-failure -R "(startup_jsonl_app_contract_test)"` | ❌ Wave 0 | ⬜ pending |
| 46-02-02 | 02 | 2 | JSON-02, JSON-04 | integration | `ctest --test-dir build-vulkan -C Release --output-on-failure -R "(startup_jsonl_app_contract_test|startup_jsonl_import_controller_test|jsonl_flat_observer_manual_refresh_test|jsonl_flat_observer_auto_safety_test|embed_launch_config_test)"` | ❌ Wave 0 | ⬜ pending |
| 46-03-01 | 03 | 3 | JSON-02 | runtime/regression | `ctest --test-dir build-vulkan -C Release --output-on-failure -R "(startup_jsonl_import_controller_test|jsonl_flat_observer_manual_refresh_test|jsonl_flat_observer_auto_safety_test)"` | ❌ Wave 0 | ⬜ pending |
| 46-03-02 | 03 | 3 | JSON-02, JSON-04 | phase gate | `ctest --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/tests/embed_launch_config_test.c` — extend parser coverage for `--jsonl-live-refresh` success/failure/default-off cases.
- [ ] `src/tests/startup_jsonl_import_controller_test.c` — cover default-off startup behavior and opt-in observer-contract bridging.
- [ ] `src/tests/startup_jsonl_app_contract_test.c` — extend source-contract coverage if overlay/root-status wiring grows in `app.c`.
- [ ] Use `build-vulkan` as the authoritative Windows CTest directory unless `build/` is explicitly regenerated.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Startup `--jsonl-live-refresh` keeps refreshing in a real viewer session after file edits | JSON-02, JSON-04 | The current repo test suite does not fully exercise live file-change events through the standalone app window | Launch mdCAD with `--jsonl <absolute-path> --jsonl-live-refresh`, edit the file on disk, and confirm the imported root refreshes without reopening the app. |
| Plain startup `--jsonl` stays default-off for live refresh | JSON-02 | The user-visible no-refresh behavior is easiest to confirm through a real launched viewer session | Launch with `--jsonl <absolute-path>` only, edit the file, and confirm nothing auto-refreshes until the future opt-in flag is supplied. |

---

## Validation Sign-Off

- [x] All planned tasks have `<automated>` verify or explicit Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 identifies all new/extended test surfaces
- [x] No watch-mode flags
- [x] Per-task fast smoke feedback latency target < 30s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** complete
