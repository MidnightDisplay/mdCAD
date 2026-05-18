---
phase: 51
slug: unsupported-platform-contract
status: draft
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-18
---

# Phase 51 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | xUnit 2.5.3 + Microsoft.NET.Test.Sdk 17.8.0 |
| **Config file** | none |
| **Quick run command** | `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj --no-restore -v minimal --filter "FullyQualifiedName~UnsupportedMdCadEmbedBackendTests"` |
| **Full suite command** | `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj --no-restore -v minimal` |
| **Estimated runtime** | single-class filters: ~20-30s warm; combined shell/coordinator/backend lane: ~35-45s and used only when a task changes their wiring together |

---

## Sampling Rate

- **After every task commit:** Run the exact task-level command from the map below; default to the smallest impacted class filter instead of the phase-wide two-class lane.
- **After every plan wave:** Run `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj --no-restore -v minimal`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 120 seconds

### Nyquist note

- Most Phase 51 tasks can use a single test class filter and should stay within the 30-second target after the first warm build.
- `51-03-01` intentionally uses a three-class lane because shell wiring is only safe when backend selection, coordinator blocking, and control warning precedence stay aligned; that broader loop is the minimum safe feedback path for that task.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 51-01-01 | 01 | 0 | PLAT-01, PLAT-03 | unit (red-first) | `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj --no-restore -v minimal --filter "FullyQualifiedName~UnsupportedMdCadEmbedBackendTests"` | `samples/avalonia-mdcad-control.tests/UnsupportedMdCadEmbedBackendTests.cs` | ⬜ pending |
| 51-01-02 | 01 | 0 | PLAT-02, PLAT-03 | unit (red-first) | `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj --no-restore -v minimal --filter "FullyQualifiedName~MdCadSessionCoordinatorTests"` | `samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs` | ⬜ pending |
| 51-02-01 | 02 | 1 | PLAT-01, PLAT-03 | unit | `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj --no-restore -v minimal --filter "FullyQualifiedName~UnsupportedMdCadEmbedBackendTests"` | `samples/avalonia-mdcad-control/Host/UnsupportedMdCadEmbedBackend.cs` | ⬜ pending |
| 51-02-02 | 02 | 1 | PLAT-02, PLAT-03 | unit | `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj --no-restore -v minimal --filter "FullyQualifiedName~UnsupportedMdCadEmbedBackendTests|FullyQualifiedName~MdCadSessionCoordinatorTests"` | `samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs` | ⬜ pending |
| 51-03-01 | 03 | 2 | PLAT-01, PLAT-02 | unit | `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj --no-restore -v minimal --filter "FullyQualifiedName~MdCadEmbeddedControlTests|FullyQualifiedName~UnsupportedMdCadEmbedBackendTests|FullyQualifiedName~MdCadSessionCoordinatorTests"` | `samples/avalonia-mdcad-control.tests/MdCadEmbeddedControlTests.cs` | ⬜ pending |
| 51-03-02 | 03 | 2 | PLAT-01, PLAT-02, PLAT-03 | unit | `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj --no-restore -v minimal --filter "FullyQualifiedName~MdCadEmbeddedControlTests"` | `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml` | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `samples/avalonia-mdcad-control.tests/UnsupportedMdCadEmbedBackendTests.cs` — new unsupported-backend contract tests for PLAT-01 and PLAT-03
- [ ] `samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs` additions — no-launch reconcile coverage for PLAT-02
- [ ] Wave 0 test commands above run as the red-first proof lane before Plan 51-02 implementation

---

## Manual-Only Verifications

All phase behaviors should have automated verification.

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] No task uses `MISSING` verification placeholders
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency justified per task map and stays within the smallest safe lane
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
