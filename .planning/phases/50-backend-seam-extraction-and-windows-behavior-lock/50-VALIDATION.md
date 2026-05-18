---
phase: 50
slug: backend-seam-extraction-and-windows-behavior-lock
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-05-18
---

# Phase 50 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | xUnit 2.5.3 + Microsoft.NET.Test.Sdk 17.8.0 |
| **Config file** | `samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj` |
| **Quick run command** | `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadSessionCoordinatorTests|FullyQualifiedName~MdCadLaunchSnapshotTests"` |
| **Full suite command** | `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj -c Release && dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` |
| **Estimated runtime** | ~60 seconds |

---

## Sampling Rate

- **After every task commit:** Run `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadSessionCoordinatorTests|FullyQualifiedName~MdCadLaunchSnapshotTests"`
- **After every plan wave:** Run `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj -c Release && dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 120 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| TBD-runtime-launch-proof | TBD | TBD | WPRS-01 | unit + manual harness | `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj -c Release` plus manual `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` and host run | ❌ W0 backend-specific proof | ⬜ pending |
| TBD-relaunch-status-proof | TBD | TBD | WPRS-02 | unit + manual harness | `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadSessionCoordinatorTests"` plus manual host lifecycle check | ⚠️ partial | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs` — pin preserved runtime path, working directory, and required launch args for the extracted Windows backend
- [ ] `samples/avalonia-mdcad-control.tests/MdCadRuntimeResolverTests.cs` — pin the `AppContext.BaseDirectory\mdcad-runtime\mdCAD.exe` lookup contract explicitly
- [ ] Manual regression checklist or artifact for `samples/avalonia-host` — verify attach, stop, relaunch, and sealed/diagnostic equivalence after extraction

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Child-HWND attach, close, relaunch, and resize behavior in the diagnostic harness | WPRS-01, WPRS-02 | Current repo proof still depends on real host runtime behavior rather than a fully automated HWND integration test | Build `samples/avalonia-host`, launch it on Windows, start the embedded session, confirm attach succeeds, close and relaunch the session, resize the host window, and verify the viewer stays attached with the existing status behavior |
| Sealed vs diagnostic surface equivalence after backend extraction | WPRS-02 | Presentation and lifecycle wiring are coupled in the public control and are not fully covered by unit tests today | Exercise both presentation modes through the existing host controls and confirm no user-visible behavior drift in warning/status text or session lifecycle |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 120s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
