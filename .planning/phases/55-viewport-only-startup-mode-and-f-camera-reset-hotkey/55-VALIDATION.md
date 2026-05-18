---
phase: 55
slug: viewport-only-startup-mode-and-f-camera-reset-hotkey
status: draft
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-18
---

# Phase 55 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | existing xUnit control/backend tests + CTest-backed native contract/unit tests |
| **Config file** | `samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj` and `src/CMakeLists.txt` |
| **Quick run command** | `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCad" && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests" && ctest --test-dir build-vulkan -C Release -R embed_launch_config_test --output-on-failure && ctest --test-dir build-vulkan -C Release -R embed_layout_state_test --output-on-failure` |
| **Full suite command** | `dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCad" && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests" && ctest --test-dir build-vulkan -C Release -R embed_launch_config_test --output-on-failure && ctest --test-dir build-vulkan -C Release -R embed_layout_state_test --output-on-failure && ctest --test-dir build-vulkan -C Release -R viewport_only_app_contract_test --output-on-failure && ctest --test-dir build-vulkan -C Release -R camera_shortcut_app_contract_test --output-on-failure` |
| **Estimated runtime** | ~180 seconds |

---

## Sampling Rate

- **After every task commit:** Run the smallest impacted command from the map below
- **After every plan wave:** Run `dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCad" && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests" && ctest --test-dir build-vulkan -C Release -R embed_launch_config_test --output-on-failure && ctest --test-dir build-vulkan -C Release -R embed_layout_state_test --output-on-failure && ctest --test-dir build-vulkan -C Release -R viewport_only_app_contract_test --output-on-failure && ctest --test-dir build-vulkan -C Release -R camera_shortcut_app_contract_test --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 180 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 55-01-01 | 01 | 1 | P55-01 | unit | `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadLaunchSnapshotTests" && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadEmbeddedControlTests"` | `samples/avalonia-mdcad-control.tests/MdCadLaunchSnapshotTests.cs` | ⬜ pending |
| 55-01-02 | 01 | 1 | P55-01 | unit | `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests" && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadSessionCoordinatorTests"` | `samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs` | ⬜ pending |
| 55-02-01 | 02 | 2 | P55-02 | native unit | `ctest --test-dir build-vulkan -C Release -R embed_launch_config_test --output-on-failure && ctest --test-dir build-vulkan -C Release -R embed_layout_state_test --output-on-failure` | `src/tests/embed_launch_config_test.c` | ⬜ pending |
| 55-02-02 | 02 | 2 | P55-03 | native contract | `ctest --test-dir build-vulkan -C Release -R embed_layout_state_test --output-on-failure && ctest --test-dir build-vulkan -C Release -R viewport_only_app_contract_test --output-on-failure` | `src/tests/viewport_only_app_contract_test.c` | ⬜ pending |
| 55-03-01 | 03 | 3 | P55-04 | native contract | `ctest --test-dir build-vulkan -C Release -R viewport_only_app_contract_test --output-on-failure && ctest --test-dir build-vulkan -C Release -R camera_shortcut_app_contract_test --output-on-failure` | `src/tests/camera_shortcut_app_contract_test.c` | ⬜ pending |
| 55-03-02 | 03 | 3 | P55-04 | integration smoke | `dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCad" && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests" && ctest --test-dir build-vulkan -C Release -R embed_launch_config_test --output-on-failure && ctest --test-dir build-vulkan -C Release -R embed_layout_state_test --output-on-failure && ctest --test-dir build-vulkan -C Release -R viewport_only_app_contract_test --output-on-failure && ctest --test-dir build-vulkan -C Release -R camera_shortcut_app_contract_test --output-on-failure` | `.planning/phases/55-viewport-only-startup-mode-and-f-camera-reset-hotkey/55-VALIDATION.md` | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements. Phase 55 does not require a separate Wave 0 plan:

- `55-02` creates `src/tests/viewport_only_app_contract_test.c` and registers it in `src/CMakeLists.txt` before app-side viewport-only behavior lands.
- `55-03` creates `src/tests/camera_shortcut_app_contract_test.c` and registers it in `src/CMakeLists.txt` before the `F` shortcut lands.

---

## Manual-Only Verifications

All phase behaviors have automated verification. Viewport-only embedded mode persists through its own dedicated embedded layout store, so no separate manual layout checklist is required for planning.

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] No standalone Wave 0 plan is required; new tests are created in-task before implementation
- [x] No watch-mode flags
- [x] Feedback latency < 180s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved 2026-05-18
