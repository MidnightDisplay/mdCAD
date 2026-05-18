---
phase: 54
slug: docs-and-onboarding-truthfulness
status: complete
nyquist_compliant: true
wave_0_complete: true
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
| 54-02-01 | 02 | 2 | PROOF-02 | docs grep | `rg -n "net10\\.0|Windows-only|unsupported|JsonlPath" samples/avalonia-mdcad-control/QUICKSTART.md` | `samples/avalonia-mdcad-control/QUICKSTART.md` | ✅ green |
| 54-02-02 | 02 | 2 | PROOF-02 | docs grep + build smoke | `rg -n "net10\\.0|Windows-only|unsupported|replace|JsonlPath" samples/avalonia-mdcad-control/QUICKSTART.md samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs && dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release` | `samples/avalonia-mdcad-control/QUICKSTART.md` | ✅ green |
| 54-03-01 | 03 | 3 | PROOF-02 | validation ledger grep | `Select-String -Path ".\.planning\phases\54-docs-and-onboarding-truthfulness\54-VALIDATION.md" -Pattern "Phase 53|53-MANUAL-CHECKLIST|53-03-SUMMARY|PROOF-02"` | `.planning/phases/54-docs-and-onboarding-truthfulness/54-VALIDATION.md` | ✅ green |
| 54-03-02 | 03 | 3 | PROOF-02 | docs proof smoke + diagnostic host build | `rg -n "net10\\.0|Windows-only|unsupported|minimal host|diagnostic host" README.md samples/avalonia-mdcad-control/QUICKSTART.md && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release` | `.planning/phases/54-docs-and-onboarding-truthfulness/54-VALIDATION.md` | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing build/test infrastructure already covers the runtime-boundary truth this phase needs to reference, including the unsupported-platform behavior already exercised by `MdCadEmbeddedControlTests` and `UnsupportedMdCadEmbedBackendTests`. Phase 54 adds wording validation on top of those existing proof surfaces and does not require new test infrastructure.

Phase 54 reuses these existing proof authorities for `PROOF-02` rather than replacing them:

- Plain `net10.0` compile/build proof remains `samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj`.
- Windows runtime proof remains `samples/avalonia-host/AvaloniaHost.csproj` plus the existing runtime-boundary xUnit suites.
- The approved manual runtime authority remains `.planning/phases/53-consumer-proof-and-windows-regression-closure/53-MANUAL-CHECKLIST.md`.
- The cross-phase proof split is summarized in `.planning/phases/53-consumer-proof-and-windows-regression-closure/53-03-SUMMARY.md`.

---

## Manual-Only Verifications

None expected. Phase 54 should reuse the already-approved Phase 53 manual checklist as a referenced proof artifact instead of introducing a new manual gate. The only manual Windows runtime proof required for this milestone already passed in Phase 53 and remains recorded in `.planning/phases/53-consumer-proof-and-windows-regression-closure/53-MANUAL-CHECKLIST.md`.

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 180s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved 2026-05-18
