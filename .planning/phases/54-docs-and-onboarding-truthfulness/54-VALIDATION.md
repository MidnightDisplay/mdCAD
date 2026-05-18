---
phase: 54
slug: docs-and-onboarding-truthfulness
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-05-18
---

# Phase 54 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | `rg` wording checks + `dotnet build` smoke lanes + existing xUnit unsupported/runtime-boundary suites |
| **Config file** | none |
| **Quick run command** | `rg -n "net10\\.0|Windows-only|unsupported|minimal host|diagnostic host" README.md samples/avalonia-mdcad-control/QUICKSTART.md` |
| **Full suite command** | `rg -n "net10\\.0|Windows-only|unsupported|minimal host|diagnostic host" README.md samples/avalonia-mdcad-control/QUICKSTART.md && dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~RuntimeRefresh|FullyQualifiedName~MdCadEmbeddedControlTests|FullyQualifiedName~UnsupportedMdCadEmbedBackendTests|FullyQualifiedName~MdCadRuntimeResolverTests|FullyQualifiedName~MdCadSessionCoordinatorTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests"` |
| **Estimated runtime** | ~180 seconds |

---

## Sampling Rate

- **After every task commit:** Run the smallest impacted command from the map below
- **After every plan wave:** Run `rg -n "net10\\.0|Windows-only|unsupported|minimal host|diagnostic host" README.md samples/avalonia-mdcad-control/QUICKSTART.md && dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~RuntimeRefresh|FullyQualifiedName~MdCadEmbeddedControlTests|FullyQualifiedName~UnsupportedMdCadEmbedBackendTests|FullyQualifiedName~MdCadRuntimeResolverTests|FullyQualifiedName~MdCadSessionCoordinatorTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests"` 
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 180 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 54-01-01 | 01 | 1 | PROOF-02 | docs grep | `rg -n "net10\\.0|runtime proof|minimal host|diagnostic host" README.md` | `README.md` | ✅ green |
| 54-01-02 | 01 | 1 | PROOF-02 | docs grep | `rg -n "net10\\.0|Windows-only|unsupported|runtime proof|minimal host|diagnostic host" README.md` | `README.md` | ✅ green |
| 54-02-01 | 02 | 2 | PROOF-02 | docs grep | `rg -n "net10\\.0|Windows-only|unsupported|JsonlPath" samples/avalonia-mdcad-control/QUICKSTART.md` | `samples/avalonia-mdcad-control/QUICKSTART.md` | ⬜ pending |
| 54-02-02 | 02 | 2 | PROOF-02 | docs grep + build smoke | `rg -n "net10\\.0|Windows-only|unsupported|replace|JsonlPath" samples/avalonia-mdcad-control/QUICKSTART.md samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs && dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release` | `samples/avalonia-mdcad-control/QUICKSTART.md` | ⬜ pending |
| 54-03-01 | 03 | 3 | PROOF-02 | validation ledger grep | `Select-String -Path ".\.planning\phases\54-docs-and-onboarding-truthfulness\54-VALIDATION.md" -Pattern "Phase 53|53-MANUAL-CHECKLIST|53-03-SUMMARY|PROOF-02"` | `.planning/phases/54-docs-and-onboarding-truthfulness/54-VALIDATION.md` | ⬜ pending |
| 54-03-02 | 03 | 3 | PROOF-02 | docs proof smoke + diagnostic host build | `rg -n "net10\\.0|Windows-only|unsupported|minimal host|diagnostic host" README.md samples/avalonia-mdcad-control/QUICKSTART.md && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release` | `.planning/phases/54-docs-and-onboarding-truthfulness/54-VALIDATION.md` | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing build/test infrastructure already covers the runtime-boundary truth this phase needs to reference, including the unsupported-platform behavior already exercised by `MdCadEmbeddedControlTests` and `UnsupportedMdCadEmbedBackendTests`. Phase 54 adds wording validation on top of those existing proof surfaces and does not require new test infrastructure.

---

## Manual-Only Verifications

None expected. Phase 54 should reuse the already-approved Phase 53 manual checklist as a referenced proof artifact instead of introducing a new manual gate.

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 180s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
