---
phase: 52
slug: plain-net10-control-compatibility
status: draft
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-18
---

# Phase 52 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | xUnit 2.5.3 + Microsoft.NET.Test.Sdk 17.8.0 plus `dotnet build` smoke lanes |
| **Config file** | none |
| **Quick run command** | `dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release` |
| **Full suite command** | `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release -v minimal && dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release` |
| **Estimated runtime** | warm build/test lane: ~60 seconds |

---

## Sampling Rate

- **After every task commit:** Run the exact task-level command from the map below; use the smallest impacted build/test lane rather than the full regression bundle.
- **After every plan wave:** Run `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release -v minimal && dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 120 seconds

### Nyquist note

- Phase 52 is primarily a compile-time compatibility correction, so the smallest safe feedback lane is usually a build of the plain-`net10.0` minimal host.
- Control-unit coverage from Phase 51 must remain green after any TFM/project edits so the widened compile-time contract does not regress unsupported-platform or Windows-preservation behavior.
- `HOSTC-02` still needs one manual startup smoke because Phase 52 should not invent a new UI automation harness.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 52-01-01 | 01 | 1 | HOSTC-01 | build smoke | `dotnet build .\samples\avalonia-mdcad-control\MdCad.Avalonia.Control.csproj -c Release` | `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj` | ✅ green |
| 52-02-01 | 02 | 2 | HOSTC-01, HOSTC-02 | build smoke | `dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release` | `samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj` | ✅ green |
| 52-03-01 | 03 | 3 | HOSTC-01, HOSTC-02 | regression build + unit | `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release -v minimal && dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release` | `samples/avalonia-host/AvaloniaHost.csproj` | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Plain `net10.0` minimal host starts and instantiates `MdCadEmbeddedControl` without startup crash | HOSTC-02 | No existing automated UI harness should be added in this phase | On Windows, run `dotnet run --project .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release` and confirm the app opens without type-load/startup failure and the control instantiates successfully. On unsupported platforms, confirm the app still opens and the control shows the existing unsupported-platform presentation instead of crashing. |

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
