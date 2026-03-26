---
phase: 03
slug: macos-core-transform-migration
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-24
---

# Phase 03 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | other |
| **Config file** | none — Phase 3 reuses the existing standalone native harness and custom CMake targets |
| **Quick run command** | `cmake -B build -G Ninja && ninja -C build mdcad_math_harness && ./build/bin/mdcad_math_harness --mode compare --strict` |
| **Full suite command** | `cmake -B build -G Ninja && ninja -C build math-validation` |
| **Estimated runtime** | ~60 seconds |

---

## Sampling Rate

- **After every task commit:** Run `cmake -B build -G Ninja && ninja -C build mdcad_math_harness && ./build/bin/mdcad_math_harness --mode compare --strict`
- **After every plan wave:** Run `cmake -B build -G Ninja && ninja -C build math-validation`
- **Before `$gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 60 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 03-01-01 | 01 | 1 | HOT-01 | static | `rg -n "orbit_camera_get_eye_position_cglm|orbit_camera_get_view_matrix_cglm|glms_lookat_rh_zo" src/orbit_camera.h` | ✅ | ⬜ pending |
| 03-01-02 | 01 | 1 | HOT-01 | build+runtime | `cmake -B build -G Ninja && ninja -C build mdcad_math_harness && ./build/bin/mdcad_math_harness --mode compare --strict | rg -n "^COMPARE PASS (orbit-camera-view|view-projection-roundtrip)"` | ✅ | ⬜ pending |
| 03-02-01 | 02 | 2 | HOT-02 | static | `rg -n "glm_translate_make|glm_mat4_mul|ecs_scene_transform_point_world|glm_mat4_mulv3" src/components/transform_comp.h src/ecs/ecs_scene.h` | ✅ | ⬜ pending |
| 03-02-02 | 02 | 2 | HOT-02 | build+runtime | `cmake -B build -G Ninja && ninja -C build mdcad_math_harness && ./build/bin/mdcad_math_harness --mode compare --strict | rg -n "^COMPARE PASS (transform-compose|hierarchy-world-transform)"` | ✅ | ⬜ pending |
| 03-03-01 | 03 | 3 | HOT-01, HOT-02 | build | `cmake -B build -G Ninja && ninja -C build math-validation && ./build/bin/mdcad_math_harness --list | rg -n "hierarchy-world-transform"` | ✅ | ⬜ pending |
| 03-03-02 | 03 | 3 | HOT-01, HOT-02 | static | `rg -n "Phase 3 macOS parity smoke|orbit/pan/zoom|math-validation" docs/QUICKSTART.md` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| macOS camera and scene parity smoke | HOT-01, HOT-02 | Automated compare cases cannot fully measure interaction feel or visible frame stability | After `ninja -C build math-validation`, run `./build/bin/mdCAD`, orbit with left-drag, pan with shift+left or middle-drag, zoom with the wheel, and confirm visible geometry stays stable while moving the camera. If parented entities are already available in the current scene, confirm child geometry follows the parent without visible drift. |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
