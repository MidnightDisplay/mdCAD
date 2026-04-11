---
phase: 34
slug: deterministic-v1-5-closure-gate
status: active
nyquist_compliant: true
wave_0_complete: true
created: 2026-04-11
---

# Phase 34 — Validation Strategy

> Deterministic closure contract for v1.5 sign-off (`DIAG-03`).

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + native C test executables |
| **Config file** | `src/CMakeLists.txt` |
| **Quick run command** | `ctest --test-dir build-vulkan -C Release -R "scene_solver_trigger|scene_solver_pass_policy|scene_solver_diagnostics" --output-on-failure` |
| **Full suite command** | Canonical 7-test gate command (see locked command definition below) |
| **Estimated runtime** | Build-dependent; execute in Windows Developer PowerShell with MSVC toolchain loaded |

---

## Canonical Deterministic Gate Contract (Locked)

### Command Definitions

- **Build command (mandatory before closure gate):**  
  `cmake --build build-vulkan --config Release`
- **Canonical 7-test gate command (locked, reuse verbatim):**  
  `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure`

### Ordered Closure Sequence (D-01..D-04)

1. Run build command once.
2. Run canonical 7-test gate command once (**Baseline**).
3. Immediately rerun the exact same canonical 7-test gate command (**Immediate rerun**).

### Deterministic Failure Policy (D-07, D-08)

- Closure is **failed** if build fails.
- Closure is **failed** if baseline run fails or flakes.
- Closure is **failed** if immediate rerun fails or flakes.
- Closure is **failed** if baseline and rerun outcomes diverge in any way.
- **Flakiness equals failure**; no “rerun until green” acceptance.

### Evidence Policy (D-05, D-06)

For build, baseline, and immediate rerun, capture:
- exact command string, and
- concise result summary (`passed/failed`, count summary, key diagnostic pointer if failing).

Raw logs are optional unless needed for debugging.

### Scope Boundary (D-02)

This closure contract is **Windows Vulkan only**.  
Cross-platform closure expansion is explicitly deferred.

---

## Sampling Rate

- **After plan task commits in this phase:** run quick command when task touches test-selection/evidence-contract surfaces.
- **Before final sign-off:** run full Windows Vulkan closure sequence (build + baseline + immediate rerun).
- **Max feedback latency target:** command-level checks immediate; build/test runtime variable by machine.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 34-01-01 | 01 | 1 | DIAG-03 | contract string check | `Select-String -Path .planning/phases/34-deterministic-v1-5-closure-gate/34-VALIDATION.md -Pattern 'cmake --build build-vulkan --config Release','scene_solver_contract\|scene_solver_pass_policy\|scene_solver_diagnostics\|scene_solver_trigger\|scene_solver_drag\|endpoint_pick\|script_roundtrip_tests','Windows Vulkan','flakiness.*failure','baseline.*rerun'` | ✅ | ⬜ pending |
| 34-01-02 | 01 | 1 | DIAG-03 | artifact evidence contract | `Select-String -Path .planning/phases/34-deterministic-v1-5-closure-gate/34-VERIFICATION.md -Pattern 'cmake --build build-vulkan --config Release','Baseline','Immediate rerun','scene_solver_contract\|scene_solver_pass_policy\|scene_solver_diagnostics\|scene_solver_trigger\|scene_solver_drag\|endpoint_pick\|script_roundtrip_tests','Outcome parity','DIAG-03'` | ⬜ (until verification exists) | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] Existing CTest/Windows Vulkan infrastructure covers phase requirement.
- [x] No additional framework/package installation required for this phase.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Execute Windows Vulkan closure sequence when local shell lacks MSVC environment | DIAG-03 | Requires Developer PowerShell or equivalent MSVC-enabled shell context | Run build + baseline + immediate rerun commands exactly as locked; then update `34-VERIFICATION.md` with concise command/result summaries. |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or pre-existing infrastructure support
- [x] Sampling continuity maintained
- [x] Wave 0 requirements satisfied
- [x] No watch-mode flags
- [x] `nyquist_compliant: true` set in frontmatter
- [x] Deterministic anti-flake policy explicitly documented (flakiness = failure)
- [x] Windows Vulkan-only scope explicitly documented

**Approval:** ready (contract locked for execution evidence capture in `34-VERIFICATION.md`)
