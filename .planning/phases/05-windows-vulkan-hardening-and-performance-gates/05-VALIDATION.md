---
phase: 05
slug: windows-vulkan-hardening-and-performance-gates
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-25
---

# Phase 05 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CMake native targets + `mdcad_math_harness` compare/bench gates |
| **Config file** | `src/CMakeLists.txt` (targets), `src/math_harness.c` (gating cases) |
| **Quick run command** | `./build/bin/mdcad_math_harness --mode compare --strict` |
| **Full suite command** | `cmake -B build -G Ninja && ninja -C build math-validation` |
| **Estimated runtime** | ~60-180 seconds |

---

## Sampling Rate

- **After every task commit:** Run strict compare on the active platform harness build.
- **After every plan wave:** Run platform-appropriate full validation target.
- **Before `$gsd-verify-work`:** Full suite must be green on macOS and Windows MSVC Vulkan hard gate.
- **Max feedback latency:** 240 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 05-01-01 | 01 | 1 | HOT-04 | strict compare + runtime smoke evidence | `.\build-vulkan\bin\Release\mdcad_math_harness.exe --mode compare --strict` | ✅ | ⬜ pending |
| 05-01-02 | 01 | 1 | HOT-04 | Vulkan pick/readback hotspot validation | `cmake --build build-vulkan --config Release --target math-validation` | ✅ | ⬜ pending |
| 05-02-01 | 02 | 2 | PERF-02 | per-case benchmark gate (macOS) | `./build/bin/mdcad_math_harness --mode bench --iterations 20000` | ✅ | ⬜ pending |
| 05-02-02 | 02 | 2 | PERF-03 | per-case benchmark gate (Windows MSVC Vulkan) | `.\build-vulkan\bin\Release\mdcad_math_harness.exe --mode bench --iterations 20000` | ✅ | ⬜ pending |
| 05-03-01 | 03 | 3 | HOT-04, PERF-02, PERF-03 | regression re-check + closure | `math-validation` on both native targets with evidence bundle complete | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] Confirm benchmark baselines exist for macOS Metal and Windows MSVC Vulkan under phase evidence tree.
- [ ] Add/verify deterministic benchmark evaluation helper for per-case `%` slowdown and rerun policy.
- [ ] Ensure evidence directories and command log capture process are documented and repeatable.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Camera orbit/pan/zoom fluidity under Vulkan | HOT-04 | Interaction smoothness not fully encoded by harness | Run `mdCAD.exe` (MSVC Vulkan), navigate dense scene, record hitch/jitter notes |
| Pick hover/click reliability on thin geometry | HOT-04 | GPU readback behavior is frame-time and scene dependent | Validate hover/click on lines/points across camera movement, capture pass/fail checklist |
| Gizmo axis/plane drag stability and vertex drag correctness | HOT-04 | Drag feel and continuity are user-facing | Exercise axis + plane drags and geometry vertex edits, confirm no jumps/stale picks |
| Undo/redo after interaction-heavy edits | HOT-04 | Sequence correctness across UI interactions | Perform drag/edit operations, then undo/redo cycles, verify state restoration |
| Import/save/reload sanity after Vulkan hardening changes | HOT-04 | End-to-end flow validation spans multiple subsystems | Import sample scene, save, reload, and verify geometry + interaction state |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 240s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
