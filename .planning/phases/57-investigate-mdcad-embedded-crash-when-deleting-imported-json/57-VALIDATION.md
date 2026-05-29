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
| **Quick run command** | `cmake --build .\build --target startup_jsonl_delete_regression_test win32_embed_diagnostics_test -j 4 && ctest --test-dir .\build -C Debug -R "startup_jsonl_delete_regression_test|win32_embed_diagnostics_test" --output-on-failure && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadEmbeddedControlTests" && dotnet test .\samples\avalonia-host.tests\AvaloniaHost.Tests.csproj -c Release --filter "FullyQualifiedName~HarnessStatus|FullyQualifiedName~MainWindow" && dotnet test .\samples\wpf-mdcad-control.tests\MdCad.Wpf.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadEmbeddedControlTests" && dotnet test .\samples\wpf-host.tests\WpfHost.Tests.csproj -c Release --filter "FullyQualifiedName~HarnessStatus|FullyQualifiedName~MainWindow" && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release && dotnet build .\samples\wpf-host\WpfHost.csproj -c Release && dotnet build .\samples\wpf-host-minimal\WpfHostMinimal.csproj -c Release` |
| **Full suite command** | `cmake --build .\build --target startup_jsonl_delete_regression_test win32_embed_diagnostics_test -j 4 && ctest --test-dir .\build -C Debug -R "startup_jsonl_delete_regression_test|win32_embed_diagnostics_test" --output-on-failure && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release && dotnet test .\samples\avalonia-host.tests\AvaloniaHost.Tests.csproj -c Release && dotnet test .\samples\wpf-mdcad-control.tests\MdCad.Wpf.Control.Tests.csproj -c Release && dotnet test .\samples\wpf-host.tests\WpfHost.Tests.csproj -c Release && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release && dotnet build .\samples\wpf-host\WpfHost.csproj -c Release && dotnet build .\samples\wpf-host-minimal\WpfHostMinimal.csproj -c Release` |
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
| 57-04-01 | 04 | 3 | P57-01, P57-02, P57-03, P57-04, P57-05 | integration/host/build | `cmake --build .\build --target startup_jsonl_delete_regression_test win32_embed_diagnostics_test -j 4 && ctest --test-dir .\build -C Debug -R "startup_jsonl_delete_regression_test|win32_embed_diagnostics_test" --output-on-failure && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadEmbeddedControlTests" && dotnet test .\samples\avalonia-host.tests\AvaloniaHost.Tests.csproj -c Release --filter "FullyQualifiedName~HarnessStatus|FullyQualifiedName~MainWindow" && dotnet test .\samples\wpf-mdcad-control.tests\MdCad.Wpf.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadEmbeddedControlTests" && dotnet test .\samples\wpf-host.tests\WpfHost.Tests.csproj -c Release --filter "FullyQualifiedName~HarnessStatus|FullyQualifiedName~MainWindow" && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release && dotnet build .\samples\wpf-host\WpfHost.csproj -c Release && dotnet build .\samples\wpf-host-minimal\WpfHostMinimal.csproj -c Release` | `.planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-VALIDATION.md` | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Phase 57 does not need a separate Wave 0 plan because the proof lanes appear before later waves depend on them:

- `57-01` creates the native delete-regression and embedded-diagnostics lanes before any host surfacing work.
- `57-02` and `57-03` each extend an existing control-test lane instead of creating a new subsystem after the fact.
- `57-04` only rebaselines the ledger after native, Avalonia, and WPF proof lanes already exist.

## Proof Split Notes

- **Automated native proof (`57-01`):** `startup_jsonl_delete_regression_test` covers the startup-import delete path for both plain startup import and startup import with live refresh enabled, and `win32_embed_diagnostics_test` covers the working-directory crash-log/stderr contract.
- **Automated .NET diagnostics proof (`57-02`, `57-03`):** backend/control tests cover truthful unexpected-session-loss detail capture and relay, while `samples/avalonia-host.tests` and `samples/wpf-host.tests` only prove full-host harness mirroring of the already observed detail string.
- **Manual D-05 host matrix:** interactive delete smoke stays manual because no shipped lane drives the real host UI tree/delete flow end-to-end. Full hosts require both plain and live-refresh rows because they expose both startup JSONL modes. Minimal hosts stay on the sealed/default startup path only; they do not add a second live-refresh row unless the shipped UI grows that mode later.
- **Proof assets:** full hosts ship a repo-owned proof JSONL (`samples/avalonia-host/resources/examples/sample-host-proof.jsonl`, copied into the full-host output trees). Minimal hosts still rely on `C:\dev\HoodScoop.jsonl` from their view models because this phase did not add a repo-owned minimal-host startup asset.

---

## Manual-Only Verifications

| Check | Trigger | Evidence | Status |
|-------|---------|----------|--------|
| Avalonia full host plain startup-import delete smoke | After `57-01`, `57-02`, and `57-04-01` are green on Windows | Launch `samples/avalonia-host` with `samples/avalonia-host/resources/examples/sample-host-proof.jsonl`, keep live refresh off, delete the imported root anchor and then the first imported geometry child beneath it, and confirm mdCAD stays attached with no unexpected-session-loss warning/detail. If any detail appears, record it verbatim and compare it against the truthful automated contract from `57-02`. | ✅ pass |
| Avalonia full host live-refresh delete smoke | After `57-01`, `57-02`, and `57-04-01` are green on Windows | Launch `samples/avalonia-host` with the same bundled `sample-host-proof.jsonl`, turn live refresh on, delete the imported root anchor and then the first imported geometry child, and confirm mdCAD stays attached with no unexpected-session-loss warning/detail. If any detail appears, record it verbatim and compare it against the truthful automated contract from `57-02`. | ✅ pass |
| Avalonia minimal sealed startup-import delete smoke | After `57-01`, `57-02`, and `57-04-01` are green on Windows | Use `C:\dev\HoodScoop.jsonl` from `samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs`, confirm sealed auto-start succeeds on its default startup path, delete the imported root anchor and then the first imported geometry child, and confirm the sealed warning surface stays hidden because no unexpected session loss occurred. | ✅ pass |
| WPF full host plain startup-import delete smoke | After `57-01`, `57-03`, and `57-04-01` are green on Windows | Launch `samples/wpf-host` with the repo-owned bundled `sample-host-proof.jsonl` copied under its `resources/examples` output, keep live refresh off, delete the imported root anchor and then the first imported geometry child beneath it, and confirm mdCAD stays attached with no unexpected-session-loss warning/detail. If any detail appears, record it verbatim and compare it against the truthful automated contract from `57-03`. | ✅ pass |
| WPF full host live-refresh delete smoke | After `57-01`, `57-03`, and `57-04-01` are green on Windows | Launch `samples/wpf-host` with the same bundled `sample-host-proof.jsonl`, turn live refresh on, delete the imported root anchor and then the first imported geometry child, and confirm mdCAD stays attached with no unexpected-session-loss warning/detail. If any detail appears, record it verbatim and compare it against the truthful automated contract from `57-03`. | ✅ pass |
| WPF minimal sealed startup-import delete smoke | After `57-01`, `57-03`, and `57-04-01` are green on Windows | Use `C:\dev\HoodScoop.jsonl` from `samples/wpf-host-minimal/ViewModels/MainWindowViewModel.cs`, confirm sealed auto-start succeeds on its default startup path, delete the imported root anchor and then the first imported geometry child, and confirm the sealed warning surface stays hidden because no unexpected session loss occurred. | ✅ pass |

---

## Validation Sign-Off

- [x] All auto tasks have `<automated>` verify
- [x] Sampling continuity: no 3 consecutive auto tasks without automated verify
- [x] No standalone Wave 0 plan is required; proof lanes are created before dependent waves
- [x] No watch-mode flags
- [x] Feedback latency is bounded and explicit
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** planned 2026-05-29

**Manual checkpoint closeout:** approved 2026-05-29 after rerunning all six host rows against freshly rebuilt outputs. No unexpected-session-loss warning/detail appeared during the startup-root or first-child delete flows.
