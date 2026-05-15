---
phase: 47
slug: sample-host-workflow-proof
status: complete
nyquist_compliant: true
wave_0_complete: false
created: 2026-05-15
---

# Phase 47 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Existing CTest native suite + `dotnet build` host smoke + manual Windows host checklist |
| **Config file** | `src/CMakeLists.txt`, `samples/avalonia-host/AvaloniaHost.csproj` |
| **Fast smoke command (<30s target)** | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release && ctest --test-dir build-vulkan -C Release --output-on-failure -R "(embed_launch_config_test|startup_jsonl_import_controller_test|startup_jsonl_app_contract_test)"` |
| **Quick run command** | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release && ctest --test-dir build-vulkan -C Release --output-on-failure -R "(embed_launch_config_test|startup_jsonl_import_controller_test|startup_jsonl_app_contract_test|embed_input_state_test|embed_layout_state_test)"` |
| **Full suite command** | `cmake --build build-vulkan --config Release && ctest --test-dir build-vulkan -C Release --output-on-failure && dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` |
| **Estimated fast smoke runtime** | <30 seconds target |
| **Estimated quick/full runtime** | ~60 seconds / existing full-suite runtime plus host build |

**Phase 47 note:** use the MSVC Vulkan `build-vulkan` path for native validation commands.

---

## Sampling Rate

- **After every task commit:** Run `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
- **After every plan wave:** Run `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release && ctest --test-dir build-vulkan -C Release --output-on-failure -R "(embed_launch_config_test|startup_jsonl_import_controller_test|startup_jsonl_app_contract_test|embed_input_state_test|embed_layout_state_test)"`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency (per-task smoke):** <30 seconds target
- **Wave/phase gate latency budget:** existing full-suite runtime plus host build

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 47-01-01 | 01 | 1 | HOST-02 | build/smoke | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release && ctest --test-dir build-vulkan -C Release --output-on-failure -R "(embed_launch_config_test|startup_jsonl_import_controller_test)"` | ✅ build / ❌ host checklist | ⬜ pending |
| 47-01-02 | 01 | 1 | HOST-02, HOST-03 | build/source-review | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` | ❌ Wave 0 | ⬜ pending |
| 47-02-01 | 02 | 2 | HOST-03, HOST-04 | build/lifecycle smoke | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release && ctest --test-dir build-vulkan -C Release --output-on-failure -R "(embed_launch_config_test|embed_input_state_test|embed_layout_state_test)"` | ❌ Wave 0 | ⬜ pending |
| 47-02-02 | 02 | 2 | HOST-02, HOST-03, HOST-04 | phase gate | `cmake --build build-vulkan --config Release && ctest --test-dir build-vulkan -C Release --output-on-failure && dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `samples/avalonia-host/resources/examples/*` — bundled example JSONL content copied to host output.
- [ ] `samples/avalonia-host` manual checklist artifact for bundled example resolution, launch with/without live refresh, repeated relaunch, resize/focus regression, and orphan-process inspection.
- [ ] Optional cheap static/source-contract check for example-resource resolution and `--jsonl` / `--jsonl-live-refresh` argument wiring if build smoke alone proves too weak.
- [x] Existing infrastructure covers native CTest and host `dotnet build`; no new framework install is required.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Bundled example resolves from `resources/examples`, converts to an absolute path, and launches mdCAD with `--jsonl` | HOST-02 | The host does not expose a machine-readable launch contract artifact today, and no IPC exists to assert runtime path usage | Launch the sample host, choose the bundled example flow, and confirm the status text reports the resolved example path before mdCAD is launched. |
| Host shows launch, attach, JSONL request, and live-refresh request state without pretending it confirmed import success | HOST-03 | Status text is intentionally inferred from host-owned state instead of mdCAD IPC | Launch once with live refresh off and once on, and confirm the host distinguishes launch request, child attach, JSONL requested, and live-refresh requested/disabled states. |
| Repeated launch, resize, focus, close, and relaunch cycles leave no orphaned mdCAD processes | HOST-04 | Real HWND/process lifecycle and orphan cleanup must be exercised in a real Windows host session | Repeatedly launch, interact, close session, relaunch, resize the host, and verify Process Explorer / Task Manager shows no orphaned `mdCAD.exe` after close. |

---

## Validation Sign-Off

- [x] All planned tasks have `<automated>` verify or explicit Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 identifies all planned new test/checklist surfaces
- [x] No watch-mode flags
- [x] Per-task fast smoke feedback latency target < 30s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** complete
