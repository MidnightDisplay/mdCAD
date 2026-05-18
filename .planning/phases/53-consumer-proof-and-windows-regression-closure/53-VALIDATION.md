---
phase: 53
slug: consumer-proof-and-windows-regression-closure
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-05-18
---

# Phase 53 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | xUnit 2.5.3 + `Microsoft.NET.Test.Sdk` 17.8.0 + `dotnet build` smoke lanes |
| **Config file** | none |
| **Quick run command** | `dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release` |
| **Full suite command** | `dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~RuntimeRefresh|FullyQualifiedName~MdCadSessionCoordinatorTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadRuntimeResolverTests" && dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release` |
| **Estimated runtime** | ~180 seconds |

---

## Sampling Rate

- **After every task commit:** Run the smallest impacted command from the map below
- **After every plan wave:** Run `dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~RuntimeRefresh|FullyQualifiedName~MdCadSessionCoordinatorTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadRuntimeResolverTests" && dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 180 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 53-01-01 | 01 | 1 | PROOF-01 | build smoke | `dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release` | `samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj` | ✅ green |
| 53-02-01 | 02 | 2 | WPRS-03 | integration + unit | `dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~RuntimeRefresh|FullyQualifiedName~MdCadSessionCoordinatorTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadRuntimeResolverTests"` | `samples/avalonia-host/AvaloniaHost.csproj` | ✅ green |
| 53-03-01 | 03 | 3 | WPRS-03 | manual runtime proof | `dotnet run --project .\samples\avalonia-host\AvaloniaHost.csproj -c Release` | `.planning/phases/53-consumer-proof-and-windows-regression-closure/53-MANUAL-CHECKLIST.md` | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing automated infrastructure covers the phase's build/test requirements. The manual checklist artifact is created in Plan 03 as the final Windows runtime proof record.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Real host attach, stop, and relaunch still work against the copied runtime | WPRS-03 | The repo still has no safe headless/UI automation harness for the real native child-HWND lifecycle boundary | On Windows, run `dotnet run --project .\samples\avalonia-host\AvaloniaHost.csproj -c Release`, confirm the runtime status reports a copied bundle, then use the current `StartAsync` / `StopAsync` / relaunch controls to prove attach, stop, and relaunch against the real embedded viewer. Record the result in `53-MANUAL-CHECKLIST.md`. |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 180s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
