---
phase: 30
slug: deterministic-closure-gate-windows-vulkan-solver-docs
status: active
nyquist_compliant: true
wave_0_complete: true
created: 2026-04-09
---

# Phase 30 — Validation Strategy

> Deterministic closure contract for Windows Vulkan sign-off (`V14-01`, `V14-02`).

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + native C test executables |
| **Config file** | `src/CMakeLists.txt` |
| **Quick run command** | `ctest --test-dir build-vulkan -C Release -R "scene_solver_trigger|scene_solver_pass_policy|scene_solver_diagnostics" --output-on-failure` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure` |
| **Estimated runtime** | Build-dependent; execute in Windows Developer PowerShell with MSVC toolchain loaded |

---

## Canonical Deterministic Gate Contract (Locked)

### Command Definitions

- **Build command (mandatory before closure gate):**  
  `cmake --build build-vulkan --config Release`
- **Canonical 7-test gate command (locked, reuse verbatim):**  
  `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure`

### Ordered Closure Sequence (D-03, D-04)

1. Run build command once.
2. Run canonical 7-test gate command once (**Baseline**).
3. Immediately rerun the exact same canonical 7-test gate command (**Immediate rerun**).

### Deterministic Failure Policy (D-05)

- Closure is **failed** if build fails.
- Closure is **failed** if baseline run fails or flakes.
- Closure is **failed** if immediate rerun fails or flakes.
- **Flakiness equals failure**; no “rerun until green” acceptance.

### Evidence Policy (D-06)

For build, baseline, and immediate rerun, capture:
- exact command string, and
- concise result summary (`passed/failed`, count summary, key diagnostic pointer if failing).

Raw logs are optional unless needed for debugging.

### Scope Boundary (D-07)

This closure contract is **Windows Vulkan only**.  
Cross-platform closure expansion is explicitly deferred.

---

## Sampling Rate

- **After plan task commits in this plan:** Run targeted string/contract checks against artifacts.
- **Before final sign-off:** Run full Windows Vulkan closure sequence (build + baseline + immediate rerun).
- **Max feedback latency target:** command-level checks immediate; build/test runtime variable by machine.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 30-02-01 | 02 | 2 | V14-01, V14-02 | contract doc lint | `Select-String -Path .planning/phases/30-deterministic-closure-gate-windows-vulkan-solver-docs/30-VALIDATION.md -Pattern 'cmake --build build-vulkan --config Release','scene_solver_contract\|scene_solver_pass_policy\|scene_solver_diagnostics\|scene_solver_trigger\|scene_solver_drag\|endpoint_pick\|script_roundtrip_tests','nyquist_compliant: true','flakiness.*failure','Windows Vulkan'` | ✅ | ⬜ pending |
| 30-02-02 | 02 | 2 | V14-01, V14-02 | evidence artifact contract | `Select-String -Path .planning/phases/30-deterministic-closure-gate-windows-vulkan-solver-docs/30-VERIFICATION.md -Pattern 'cmake --build build-vulkan --config Release','Baseline','Immediate rerun','scene_solver_contract\|scene_solver_pass_policy\|scene_solver_diagnostics\|scene_solver_trigger\|scene_solver_drag\|endpoint_pick\|script_roundtrip_tests','flakiness'` | ⬜ (until Task 2) | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] Existing CTest/Windows Vulkan infrastructure covers phase requirements.
- [x] No additional framework/package installation required for this plan’s doc-contract work.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Execute Windows Vulkan closure sequence when local shell lacks MSVC environment | V14-02 | Requires Developer PowerShell or equivalent MSVC-enabled shell context | Run build + baseline + immediate rerun commands exactly as locked; then update `30-VERIFICATION.md` evidence summaries with real outcomes. |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or pre-existing infrastructure support
- [x] Sampling continuity maintained
- [x] Wave 0 requirements satisfied
- [x] No watch-mode flags
- [x] `nyquist_compliant: true` set in frontmatter
- [x] Deterministic anti-flake policy explicitly documented (flakiness = failure)
- [x] Windows Vulkan-only scope explicitly documented

**Approval:** ready (contract locked for execution evidence capture in `30-VERIFICATION.md`)
