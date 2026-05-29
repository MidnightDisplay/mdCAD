---
phase: 57
slug: investigate-mdcad-embedded-crash-when-deleting-imported-json
status: planned
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-29
---

# Phase 57 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | native C regression lanes plus Avalonia/WPF xUnit diagnostics lanes and sample-host build lanes |
| **Config file** | `src/CMakeLists.txt`, `samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj`, `samples/wpf-mdcad-control.tests/MdCad.Wpf.Control.Tests.csproj` |
| **Quick run command** | `cmake --build .\build --target startup_jsonl_delete_regression_test win32_embed_diagnostics_test -j 4 && ctest --test-dir .\build -R "startup_jsonl_delete_regression_test|win32_embed_diagnostics_test" --output-on-failure && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadEmbeddedControlTests" && dotnet test .\samples\avalonia-host.tests\AvaloniaHost.Tests.csproj -c Release --filter "FullyQualifiedName~HarnessStatus|FullyQualifiedName~MainWindow" && dotnet test .\samples\wpf-mdcad-control.tests\MdCad.Wpf.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadEmbeddedControlTests" && dotnet test .\samples\wpf-host.tests\WpfHost.Tests.csproj -c Release --filter "FullyQualifiedName~HarnessStatus|FullyQualifiedName~MainWindow" && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet build .\samples\wpf-host\WpfHost.csproj -c Release` |
| **Full suite command** | `cmake --build .\build --target startup_jsonl_delete_regression_test win32_embed_diagnostics_test -j 4 && ctest --test-dir .\build -R "startup_jsonl_delete_regression_test|win32_embed_diagnostics_test" --output-on-failure && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release && dotnet test .\samples\avalonia-host.tests\AvaloniaHost.Tests.csproj -c Release && dotnet test .\samples\wpf-mdcad-control.tests\MdCad.Wpf.Control.Tests.csproj -c Release && dotnet test .\samples\wpf-host.tests\WpfHost.Tests.csproj -c Release && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release && dotnet build .\samples\wpf-host\WpfHost.csproj -c Release && dotnet build .\samples\wpf-host-minimal\WpfHostMinimal.csproj -c Release` |
| **Estimated runtime** | ~420 seconds |

---

## Sampling Rate

- **After every task commit:** Run the smallest impacted command from the map below
- **After every wave:** Run that wave's full automated command set before continuing
- **Before `/gsd-verify-work 57`:** The full suite must be green and every manual host row must be updated
- **Max feedback latency:** 420 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 57-01-01 | 01 | 1 | P57-01, P57-02 | native regression | `ctest --test-dir .\build -R startup_jsonl_delete_regression_test --output-on-failure` | `src/tests/startup_jsonl_delete_regression_test.c` | ✅ green |
| 57-01-02 | 01 | 1 | P57-03 | native diagnostics | `ctest --test-dir .\build -R win32_embed_diagnostics_test --output-on-failure` | `src/tests/win32_embed_diagnostics_test.c` | ✅ green |
| 57-02-01 | 02 | 2 | P57-04 | unit/integration | `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests"` | `samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs` | ✅ green |
| 57-02-02 | 02 | 2 | P57-04, P57-05 | unit/host/build | `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadEmbeddedControlTests" && dotnet test .\samples\avalonia-host.tests\AvaloniaHost.Tests.csproj -c Release --filter "FullyQualifiedName~HarnessStatus|FullyQualifiedName~MainWindow" && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release` | `samples/avalonia-mdcad-control.tests/MdCadEmbeddedControlTests.cs` | ✅ green |
| 57-03-01 | 03 | 2 | P57-04 | unit/integration | `dotnet test .\samples\wpf-mdcad-control.tests\MdCad.Wpf.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests"` | `samples/wpf-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs` | ✅ green |
| 57-03-02 | 03 | 2 | P57-04, P57-05 | unit/host/build | `dotnet test .\samples\wpf-mdcad-control.tests\MdCad.Wpf.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadEmbeddedControlTests" && dotnet test .\samples\wpf-host.tests\WpfHost.Tests.csproj -c Release --filter "FullyQualifiedName~HarnessStatus|FullyQualifiedName~MainWindow" && dotnet build .\samples\wpf-host\WpfHost.csproj -c Release` | `samples/wpf-mdcad-control.tests/MdCadEmbeddedControlTests.cs` | ✅ green |
| 57-04-01 | 04 | 3 | P57-01, P57-02, P57-03, P57-04, P57-05 | integration/host/build | `ctest --test-dir .\build -R "startup_jsonl_delete_regression_test|win32_embed_diagnostics_test" --output-on-failure && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadEmbeddedControlTests" && dotnet test .\samples\avalonia-host.tests\AvaloniaHost.Tests.csproj -c Release --filter "FullyQualifiedName~HarnessStatus|FullyQualifiedName~MainWindow" && dotnet test .\samples\wpf-mdcad-control.tests\MdCad.Wpf.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadEmbeddedControlTests" && dotnet test .\samples\wpf-host.tests\WpfHost.Tests.csproj -c Release --filter "FullyQualifiedName~HarnessStatus|FullyQualifiedName~MainWindow" && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release && dotnet build .\samples\wpf-host\WpfHost.csproj -c Release && dotnet build .\samples\wpf-host-minimal\WpfHostMinimal.csproj -c Release` | `.planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-VALIDATION.md` | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Phase 57 does not need a separate Wave 0 plan because the proof lanes appear before later waves depend on them:

- `57-01` creates the native delete-regression and embedded-diagnostics lanes before any host surfacing work.
- `57-02` and `57-03` each extend an existing control-test lane instead of creating a new subsystem after the fact.
- `57-04` only rebaselines the ledger after native, Avalonia, and WPF proof lanes already exist.

---

## Manual-Only Verifications

| Check | Trigger | Evidence | Status |
|-------|---------|----------|--------|
| Avalonia full host plain startup-import delete smoke | After `57-01`, `57-02`, and `57-04-01` are green on Windows | Use the Phase 57 repo-owned proof JSONL if it ships; otherwise use the same external proof file documented by the phase. Launch `samples/avalonia-host` with live refresh off, delete the imported root anchor and then the first imported geometry child beneath it, and confirm neither action crashes mdCAD or surfaces an untruthful warning. | ⬜ pending |
| Avalonia full host live-refresh delete smoke | After `57-01`, `57-02`, and `57-04-01` are green on Windows | Use the same proof JSONL, launch `samples/avalonia-host` with live refresh on, delete the imported root anchor and then the first imported geometry child, and confirm mdCAD stays attached and any surfaced detail remains truthful if a forced failure is injected separately. | ⬜ pending |
| Avalonia minimal sealed startup-import delete smoke | After `57-01`, `57-02`, and `57-04-01` are green on Windows | Use the Phase 57 repo-owned proof JSONL if it ships; otherwise use `C:\dev\HoodScoop.jsonl` from `samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs`. Confirm auto-start succeeds, delete the imported root anchor and then the first imported geometry child, and confirm the sealed warning surface stays hidden on success. | ⬜ pending |
| WPF full host plain startup-import delete smoke | After `57-01`, `57-03`, and `57-04-01` are green on Windows | Use the Phase 57 repo-owned proof JSONL if it ships; otherwise use the same external proof file documented by the phase. Launch `samples/wpf-host` with live refresh off, delete the imported root anchor and then the first imported geometry child beneath it, and confirm neither action crashes mdCAD or surfaces an untruthful warning. | ⬜ pending |
| WPF full host live-refresh delete smoke | After `57-01`, `57-03`, and `57-04-01` are green on Windows | Use the same proof JSONL, launch `samples/wpf-host` with live refresh on, delete the imported root anchor and then the first imported geometry child, and confirm mdCAD stays attached and any surfaced detail remains truthful if a forced failure is injected separately. | ⬜ pending |
| WPF minimal sealed startup-import delete smoke | After `57-01`, `57-03`, and `57-04-01` are green on Windows | Use the Phase 57 repo-owned proof JSONL if it ships; otherwise use `C:\dev\HoodScoop.jsonl` from `samples/wpf-host-minimal/ViewModels/MainWindowViewModel.cs`. Confirm auto-start succeeds, delete the imported root anchor and then the first imported geometry child, and confirm the sealed warning surface stays hidden on success. | ⬜ pending |

---

## Validation Sign-Off

- [x] All auto tasks have `<automated>` verify
- [x] Sampling continuity: no 3 consecutive auto tasks without automated verify
- [x] No standalone Wave 0 plan is required; proof lanes are created before dependent waves
- [x] No watch-mode flags
- [x] Feedback latency is bounded and explicit
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** planned 2026-05-29
