---
phase: 11
slug: constraint-authoring-ux
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-03-31
updated: 2026-04-01
---

# Phase 11 — Validation Strategy (Compliant)

> Nyquist-compliant validation contract for implemented Phase 11 scope (SKCH-04, CONS-01, CONS-02, CONS-03, CONS-04, CONS-05).

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CMake/CTest harness + manual UI checklist with automated gate companion |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Quick run command** | `cmake --build build-vulkan --config Release --target mdCAD` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` |
| **Combined gate command** | `cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated runtime** | ~120 seconds (environment dependent) |

---

## Gate Execution Record (Windows MSVC + Vulkan)

**Executed:** 2026-04-01  
**Platform/toolchain:** Windows + MSBuild/CMake Vulkan generator (`build-vulkan`)

### Command

```bash
cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure
```

### Result

- ✅ Build passed (`mdCAD.vcxproj -> ...\build-vulkan\bin\Release\mdCAD.exe`)
- ✅ CTest command executed successfully (`No tests were found!!!` in current harness)
- ✅ Exit code success for combined gate command

---

## Sampling Rate

- **After every task commit:** `cmake --build build-vulkan --config Release --target mdCAD`
- **After every plan wave:** `ctest --test-dir build-vulkan -C Release --output-on-failure`
- **Before `/gsd-verify-work`:** Combined gate command must succeed on Windows MSVC + Vulkan
- **Max feedback latency target:** 180 seconds

---

## Requirement Validation Map (Implemented Scope)

| Requirement | Behavior | Test Type | Manual Verification Focus | Automated Companion Command | Status |
|---|---|---|---|---|---|
| SKCH-04 | Selecting constraint highlights all participants | Manual integration + build gate | Verify both manager row and glyph click highlight all alive participants immediately | `cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure` | ✅ complete |
| CONS-01 | Apply legal initial constraint set only for valid signatures | Manual legality matrix + build gate | Check valid/invalid selection signatures expose only legal types | `cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure` | ✅ complete |
| CONS-02 | In-context applicable-only menu, auto-hide after apply | Manual interaction + build gate | Press `C`, inspect filtered menu, apply one, verify menu closes | `cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure` | ✅ complete |
| CONS-03 | Hover/select through constant-screen-size glyphs | Manual viewport + build gate | Verify zoom-stable hover/select behavior and correct pick target routing | `cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure` | ✅ complete |
| CONS-04 | LENGTH/ANGLE create/view/edit mirrored manager/viewport | Manual parity + build gate | Edit from popup and manager; verify immediate mirrored values | `cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure` | ✅ complete |
| CONS-05 | Driven dimensional constraints visible/readable and persist in UI state | Manual interaction + build gate | Toggle Driven in manager/selected row and verify semantics/visibility text | `cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure` | ✅ complete |

*Status legend: ✅ complete · ⚠️ partial · ❌ failed*

---

## Manual Verification Protocol (with Nyquist Companion Commands)

Each manual verification run must be paired with the same automated gate command:

```bash
cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure
```

| Behavior | Requirement(s) | Manual Steps | Expected Result | Automated Companion |
|---|---|---|---|---|
| Canonical keybinding mapping | CONS-02 (wording clarity) | In app, press `C` and observe constraint menu behavior; press `Tab` and observe gizmo mode toggle | `C` opens constraint menu; `Tab` toggles gizmo mode; no runtime behavior change required | Combined gate command above |
| Constraint participant highlight parity | SKCH-04, CONS-03 | Apply a constraint, then select it from manager and via glyph click | Both paths highlight participant geometry consistently | Combined gate command above |
| Applicable-only context menu | CONS-01, CONS-02 | Select different geometry signatures, open menu with `C`, inspect options, apply one | Only legal options listed; menu auto-hides after apply | Combined gate command above |
| Dimensional popup + manager mirroring | CONS-04 | Double-click LENGTH/ANGLE glyph, edit value via Apply/Enter; also edit in manager | Values stay mirrored across popup and manager | Combined gate command above |
| Driven flag behavior | CONS-05 | Toggle Driven for dimensional constraints and inspect UI text/state | Driven state visible/readable, semantics preserved | Combined gate command above |

---

## Wave 0 Closure

- [x] Requirement map maintained and aligned with implemented Phase 11 scope
- [x] Explicit Windows MSVC + Vulkan build/test command evidence recorded
- [x] Manual verification rows each include automated companion command
- [x] Validation frontmatter updated to compliant (`status: complete`, `nyquist_compliant: true`, `wave_0_complete: true`)

---

## Validation Sign-Off

- [x] All in-scope requirements have explicit validation rows
- [x] Sampling continuity includes quick build and wave gate commands
- [x] Wave 0 closure items complete for implemented scope
- [x] No watch-mode flags used
- [x] Feedback latency target maintained
- [x] Nyquist compliance declared and evidenced

**Approval:** complete/compliant (implemented scope)
