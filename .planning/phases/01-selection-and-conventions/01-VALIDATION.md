---
phase: 01
slug: selection-and-conventions
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-24
---

# Phase 01 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | other |
| **Config file** | none — Phase 1 uses build and static verification |
| **Quick run command** | `cmake -B build -G Ninja && ninja -C build` |
| **Full suite command** | `cmake -B build -G Ninja && ninja -C build && rg -n "cglm 0\\.9\\.6|thin project-owned math entrypoint|MDCAD_MATH_CLIP_DEPTH_ZERO_TO_ONE|MDCAD_MATH_ALIGNMENT_POLICY_PERFORMANCE_FIRST" .planning/ROADMAP.md .planning/REQUIREMENTS.md .planning/STATE.md README.md docs/MATH_BACKEND_DECISION.md docs/MATH_CONVENTIONS.md src/math/cglm_entry.h src/math/math_conventions.h 2>/dev/null` |
| **Estimated runtime** | ~25 seconds |

---

## Sampling Rate

- **After every task commit:** Run the quick run command
- **After every plan wave:** Run the full suite command
- **Before `$gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 01-01-01 | 01 | 1 | FOUND-01 | static | `rg -n "cglm 0\\.9\\.6|direct cglm adoption|thin project-owned math entrypoint" .planning/ROADMAP.md .planning/REQUIREMENTS.md .planning/STATE.md README.md docs/MATH_BACKEND_DECISION.md` | ✅ | ⬜ pending |
| 01-02-01 | 02 | 2 | FOUND-01 | build | `cmake -B build -G Ninja && ninja -C build` | ✅ | ⬜ pending |
| 01-03-01 | 03 | 3 | FOUND-03 | static | `rg -n "MDCAD_MATH_CLIP_DEPTH_ZERO_TO_ONE|MDCAD_MATH_ALIGNMENT_POLICY_PERFORMANCE_FIRST|Visible behavior normalization" src/math/math_conventions.h docs/MATH_CONVENTIONS.md src/math/cglm_entry.h` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements.

---

## Manual-Only Verifications

All phase behaviors have automated verification.

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
