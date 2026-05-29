---
phase: 56
slug: create-wpf-mdcad-control-for-net10-0-windows-consumers-with-
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-28
---

# Phase 56 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | shared-core xUnit plus existing Avalonia regression lane plus WPF control/runtime-copy tests and WPF sample-host build lanes |
| **Config file** | `samples/mdcad-embed-core.tests/MdCad.Embed.Core.Tests.csproj`, `samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj`, `samples/wpf-mdcad-control.tests/MdCad.Wpf.Control.Tests.csproj` |
| **Quick run command** | `dotnet test .\samples\mdcad-embed-core.tests\MdCad.Embed.Core.Tests.csproj -c Release && dotnet test .\samples\wpf-mdcad-control.tests\MdCad.Wpf.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadEmbeddedControlTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~RuntimeRefreshIntegrationTests" && dotnet build .\samples\wpf-host\WpfHost.csproj -c Release && dotnet build .\samples\wpf-host-minimal\WpfHostMinimal.csproj -c Release` |
| **Full suite command** | `dotnet test .\samples\mdcad-embed-core.tests\MdCad.Embed.Core.Tests.csproj -c Release && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release && dotnet test .\samples\wpf-mdcad-control.tests\MdCad.Wpf.Control.Tests.csproj -c Release && dotnet build .\samples\wpf-mdcad-control\MdCad.Wpf.Control.csproj -c Release && dotnet build .\samples\wpf-host\WpfHost.csproj -c Release && dotnet build .\samples\wpf-host-minimal\WpfHostMinimal.csproj -c Release` |
| **Estimated runtime** | ~330 seconds |

---

## Sampling Rate

- **After every task commit:** Run the smallest impacted command from the map below
- **After every plan wave:** Run that wave's full automated command set before continuing
- **Before `/gsd-verify-work`:** Full suite must be green and the manual WPF lifecycle proof row must be completed
- **Max feedback latency:** 330 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 56-01-01 | 01 | 1 | P56-01 | unit | `dotnet test .\samples\mdcad-embed-core.tests\MdCad.Embed.Core.Tests.csproj -c Release` | `samples/mdcad-embed-core.tests/MdCadLaunchSnapshotTests.cs` | ✅ green |
| 56-02-01 | 02 | 2 | P56-01 | unit | `dotnet test .\samples\mdcad-embed-core.tests\MdCad.Embed.Core.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadRuntimeResolverTests|FullyQualifiedName~MdCadSessionCoordinatorTests|FullyQualifiedName~MdCadRuntimePathsTests"` | `samples/mdcad-embed-core/MdCadSessionCoordinator.cs` | ✅ green |
| 56-03-01 | 03 | 3 | P56-02 | unit/integration | `dotnet test .\samples\mdcad-embed-core.tests\MdCad.Embed.Core.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadWindowsStartInfoBuilderTests" && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests"` | `samples/mdcad-embed-core/Windows/MdCadWindowsStartInfoBuilder.cs` | ✅ green |
| 56-04-01 | 04 | 4 | P56-01 | integration | `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadEmbeddedControlTests"` | `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs` | ✅ green |
| 56-05-01 | 05 | 3 | P56-01 | build | `dotnet build .\samples\wpf-mdcad-control.tests\MdCad.Wpf.Control.Tests.csproj -c Release` | `samples/wpf-mdcad-control.tests/MdCad.Wpf.Control.Tests.csproj` | ✅ green |
| 56-06-01 | 06 | 4 | P56-01, P56-02 | unit/integration | `dotnet test .\samples\wpf-mdcad-control.tests\MdCad.Wpf.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadEmbeddedControlTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests" && dotnet build .\samples\wpf-mdcad-control\MdCad.Wpf.Control.csproj -c Release` | `samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml.cs` | ✅ green |
| 56-07-01 | 07 | 5 | P56-03 | build | `dotnet build .\samples\wpf-host\WpfHost.csproj -c Release` | `samples/wpf-host/WpfHost.csproj` | ✅ green |
| 56-08-01 | 08 | 5 | P56-04 | build | `dotnet build .\samples\wpf-host-minimal\WpfHostMinimal.csproj -c Release` | `samples/wpf-host-minimal/WpfHostMinimal.csproj` | ✅ green |
| 56-09-01 | 09 | 6 | P56-02, P56-03, P56-04 | integration | `dotnet test .\samples\wpf-mdcad-control.tests\MdCad.Wpf.Control.Tests.csproj -c Release --filter "FullyQualifiedName~RuntimeRefreshIntegrationTests"` | `samples/wpf-mdcad-control.tests/RuntimeRefreshIntegrationTests.cs` | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Phase 56 does not need a separate Wave 0 plan because the nine-plan split creates each required verification lane before later waves depend on it:

- `56-01` creates the first shared-core test lane for snapshot/presentation/support contracts.
- `56-02` extends that shared-core lane with runtime/coordinator coverage before Avalonia/WPF rewiring.
- `56-05` creates the WPF control test lane before `56-06` depends on it.
- `56-09` only updates the validation ledger after both WPF host build proofs already exist.

---

## Manual-Only Verifications

| Check | Trigger | Evidence | Status |
|-------|---------|----------|--------|
| Full WPF host manual lifecycle proof | After `56-07-01`, `56-08-01`, and `56-09-01` are green on Windows | Attach, explicit stop, relaunch after `JsonlPath`/live-refresh/viewport-only changes, bundled-proof-JSONL scenario works from output, and truthful missing/unreadable-path warning captured during `/gsd-verify-work 56`. | ⬜ pending |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] No standalone Wave 0 plan is required; test/build lanes are created before later waves consume them
- [x] No watch-mode flags
- [x] Feedback latency is bounded and explicit
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** rebaselined 2026-05-29
