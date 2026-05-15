---
phase: 48
slug: reusable-avalonia-mdcad-user-control
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-05-15
---

# Phase 48 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | other — existing CTest/MSVC Vulkan native lane plus .NET/Avalonia build smoke |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt`, `samples/avalonia-host/AvaloniaHost.csproj` |
| **Quick run command** | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` |
| **Full suite command** | `cmake --build build-vulkan --config Release && ctest --test-dir build-vulkan -C Release --output-on-failure && dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` |
| **Estimated runtime** | ~120 seconds |

---

## Sampling Rate

- **After every task commit:** Run `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
- **After every plan wave:** Run `cmake --build build-vulkan --config Release && ctest --test-dir build-vulkan -C Release --output-on-failure && dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 120 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 48-01-01 | 01 | 1 | P48-01 | build | `dotnet build samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj -c Release` | ❌ W0 | ⬜ pending |
| 48-02-01 | 02 | 1 | P48-02 | smoke | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` + output existence check for `mdcad-runtime\mdCAD.exe` | ❌ W0 | ⬜ pending |
| 48-03-01 | 03 | 2 | P48-05 | manual/UAT + optional unit | `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release` | ✅ harness / ❌ pure-logic tests W0 | ⬜ pending |
| 48-03-02 | 03 | 2 | P48-04 | manual/UAT | `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release` + `Get-Process mdCAD -ErrorAction SilentlyContinue` | ✅ harness / ❌ checklist W0 | ⬜ pending |
| 48-04-01 | 04 | 2 | P48-03 | manual/UAT | `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release` | ✅ harness / ❌ checklist W0 | ⬜ pending |
| 48-04-02 | 04 | 2 | P48-06 | manual/UAT | `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release` | ✅ harness / ❌ checklist W0 | ⬜ pending |
| 48-04-03 | 04 | 2 | P48-07 | regression | `ctest --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj` — reusable control project does not exist yet
- [ ] Managed test seam for extracted pure logic (`MdCadLaunchSnapshot`, path normalization, relaunch coordinator coalescing) — no .NET test project exists yet
- [ ] `.planning/phases/48-reusable-avalonia-mdcad-user-control/48-MANUAL-CHECKLIST.md` — new checklist needed for runtime-subfolder copy, auto-start, explicit start, sealed/diagnostic, and property-change relaunch
- [ ] Deterministic output-copy smoke check for `mdcad-runtime\mdCAD.exe` under the consuming app output

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| XAML-dropped control auto-launches and attaches mdCAD inside the hosted region | P48-03 | Real attach behavior depends on Avalonia visual-tree timing and a live Win32 child HWND | Launch the sample harness, load the reusable control in default auto-start mode, and confirm mdCAD attaches inside the control-hosted region |
| `AutoStart=false` plus explicit start works without orphaning mdCAD | P48-04 | Requires real process lifecycle observation and teardown timing | Launch the harness in explicit-start mode, trigger start from diagnostic UI/API, then close/stop and confirm `Get-Process mdCAD -ErrorAction SilentlyContinue` stays empty afterward |
| Changing `JsonlPath` / live-refresh while running triggers one safe relaunch using the latest snapshot only | P48-05 | Race safety and stale-generation suppression are hard to prove through build-only checks | Change launch-affecting properties while mdCAD is running, verify one coalesced relaunch occurs, and confirm no extra orphaned process survives |
| `sealed` mode stays bare, `diagnostic` mode shows host-owned controls/status, and bad requested JSONL stays visible | P48-06 | Presentation-mode differences and warning visibility are UI/runtime behaviors | Exercise both presentation modes in the harness, confirm diagnostic chrome only appears in diagnostic mode, and confirm bad `JsonlPath` shows a visible sealed-mode warning while leaving the viewer usable |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 120s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
