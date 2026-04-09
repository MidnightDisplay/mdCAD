# Phase 30: Deterministic Closure Gate (Windows Vulkan) + Solver Docs - Research

**Researched:** 2026-04-09  
**Domain:** Deterministic regression closure gating (CTest/Windows Vulkan) + solver architecture documentation  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
## Implementation Decisions

### Regression closure gate contract
- **D-01:** Use the exact canonical 7-test targeted suite already established in prior closure work:
  - `script_roundtrip_tests`
  - `scene_solver_contract`
  - `scene_solver_pass_policy`
  - `scene_solver_diagnostics`
  - `scene_solver_trigger`
  - `scene_solver_drag`
  - `endpoint_pick`
- **D-02:** Lock the canonical closure command regex string in Phase 30 docs/verification artifacts (no variant command patterns).
- **D-03:** Deterministic sign-off requires both a baseline pass and a mandatory immediate rerun pass using the same canonical 7-test command.
- **D-04:** Windows Vulkan closure run must include build + gate + rerun in this phase (`cmake --build build-vulkan --config Release` followed by canonical gate command and mandatory rerun).

### Determinism and failure policy
- **D-05:** Any flakiness during closure reruns is treated as failure and must be stabilized before Phase 30 closure.
- **D-06:** Verification evidence must include command plus result summaries; raw logs are optional unless needed for debugging.
- **D-07:** Closure sign-off scope remains the established Windows Vulkan path only for this phase (no cross-platform expansion).

### Solver documentation contract
- **D-08:** Create dedicated solver doc at `docs/solver/SOLVER_ARCHITECTURE.md`.
- **D-09:** Doc structure is locked as: Overview -> Solve pipeline -> Diagnostics flow -> Code anchors -> TL;DR debug primer.
- **D-10:** SDOC-02 mapping must include stage-by-stage file and key function anchors for authoring, recalc, diagnostics, and UI feedback.

### Literature and debug-primer depth
- **D-11:** Include a concise curated set of 3-6 high-signal references, each with one-line practical rationale.
- **D-12:** Prioritize practical geometric-constraint and numerical-robustness references over theory-only depth.
- **D-13:** TL;DR must include explicit "first place to look" debug entry points for failure families: unsatisfied constraints, drag rollback behavior, and pass-policy convergence stalls.

### the agent's Discretion
- Exact wording/formatting of the canonical command examples, as long as D-01..D-04 remain explicit and unambiguous.
- Exact section naming/details inside `docs/solver/SOLVER_ARCHITECTURE.md`, as long as D-08..D-13 and `SDOC-01..03` are fully covered.
- Exact verification artifact layout and summary style, as long as deterministic provenance is reproducible.

### Deferred Ideas (OUT OF SCOPE)
## Deferred Ideas

- Cross-platform closure expansion (macOS/Linux co-equal sign-off) remains a future milestone concern.
- Any solver capability additions discovered during docs/closure work remain out of scope for Phase 30.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SDOC-01 | Developer can read an easy-to-follow solver architecture overview that maps authoring, solve, diagnostics, and UI feedback flow. | Prescriptive doc structure, code-anchor map, and section contract for `docs/solver/SOLVER_ARCHITECTURE.md`. |
| SDOC-02 | Developer can use references to relevant literature and direct code-structure anchors (files/functions) to understand implementation intent. | Stage-by-stage anchor mapping and required 3-6 curated references with one-line practical rationale each. |
| SDOC-03 | Developer can use a TL;DR primer that explains how key constraints are implemented and where to start when debugging failures. | Required TL;DR debug entry points for unsatisfied constraints, drag rollback, and pass-policy stalls. |
| V14-01 | Developer can run targeted automated tests covering new line-line constraints, line ALONG semantics, tangency robustness, and active-sketch line gizmo behavior. | Canonical 7-test targeted CTest gate from `src/CMakeLists.txt` plus deterministic command contract and evidence format. |
| V14-02 | Developer can run milestone closure reruns on Windows Vulkan and obtain deterministic pass results suitable for sign-off. | Mandatory Windows Vulkan build + baseline gate + immediate rerun gate, with failure-on-flake policy and summary evidence requirements. |
</phase_requirements>

## Summary

Phase 30 is a **closure-and-documentation phase**, not a feature phase. The codebase already contains the complete targeted regression suite in `src/CMakeLists.txt` and current Windows Vulkan guidance in `docs/QUICKSTART.md` / `docs/VULKAN_WINDOWS.md`. Planning should therefore focus on: (1) enforcing one locked canonical closure command, (2) requiring immediate rerun determinism, and (3) publishing a practical solver architecture document anchored to real code paths.

The deterministic gate is already strongly established by prior closure artifacts (Phase 25 and Phase 29): strict pass/fail, no flake tolerance, and command+result summary evidence. For this phase, the highest-risk planning mistake is process drift (different regexes, partial test slices, or omitted rerun) rather than missing implementation code.

For solver docs, the strongest implementation anchor is `src/ecs/ecs_scene.h` (solve request lifecycle, pass policy controls, diagnostics, drag feasibility/projection), with `src/app.c` showing UI orchestration and feedback surfaces, and `src/constraints/constraint_types.h` capturing legality contracts. The doc should map “authoring → recalc → diagnostics → UI feedback” directly to these symbols.

**Primary recommendation:** Treat Phase 30 as a reproducibility contract: lock one canonical Windows Vulkan closure command string, require immediate rerun pass, and publish `docs/solver/SOLVER_ARCHITECTURE.md` as a code-anchored debug guide.

## Project Constraints (from copilot-instructions.md)

`./copilot-instructions.md` was not found in repository root; no additional project-specific directives were discovered from that file.

## Standard Stack

### Core
| Library/Tool | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| CMake/CTest | 4.3.1 (installed) | Build + targeted regression execution | Existing project gate is CMake-native and all 7 required tests are registered in CTest. |
| Existing native C tests (`src/tests/*.c`) | repo-local | Deterministic solver/interaction contracts | Already cover required domains (line-line, ALONG, tangency drag, endpoint/gizmo, script roundtrip). |
| Windows Vulkan build (`build-vulkan`) | VS generator `"Visual Studio 18"` + `USE_VULKAN=ON` | Required sign-off path for V14-02 | Locked by context decisions and existing docs/workflows. |

### Supporting
| Library/Tool | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Vulkan SDK | env detected: `1.4.341.1` | Vulkan toolchain/runtime dependency | Required for Windows Vulkan configure/build path. |
| Markdown docs in `docs/` | repo-local | Human-readable architecture guidance | Use for `docs/solver/SOLVER_ARCHITECTURE.md` + cross-linking from existing docs. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Targeted 7-test gate | Full `ctest` suite | Violates locked scope (D-01/D-02) and increases closure noise. |
| Windows Vulkan-only closure | Cross-platform matrix rerun | Explicitly deferred/out-of-scope for Phase 30. |
| Code-anchored architecture doc | Theory-heavy solver explainer | Fails SDOC practical-debug intent and D-10/D-13 requirements. |

**Installation:**  
No new packages are required for this phase; use existing toolchain and repository tests/docs.

## Architecture Patterns

### Recommended Project Structure
```text
.planning/phases/30-deterministic-closure-gate-windows-vulkan-solver-docs/
├── 30-RESEARCH.md          # this file
├── 30-PLAN.md / 30-xx-PLAN.md
├── 30-VALIDATION.md        # Nyquist validation map
└── 30-VERIFICATION.md      # deterministic closure evidence

docs/
└── solver/
    └── SOLVER_ARCHITECTURE.md
```

### Pattern 1: Canonical deterministic closure gate
**What:** One fixed CTest regex command for all seven closure tests, executed baseline then immediate rerun, both required to pass.  
**When to use:** Phase closure verification and sign-off evidence generation for V14-01/V14-02.  
**Example:**
```powershell
# Source: .planning/phases/25-regression-and-reliability-closure/25-VALIDATION.md
cmake --build build-vulkan --config Release
ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure
ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure
```

### Pattern 2: Solver docs mapped to runtime ownership
**What:** Explain solver flow by following ownership boundaries already in code (scene owns solver state; app triggers and surfaces diagnostics).  
**When to use:** SDOC-01..03 authoring and future debug onboarding.  
**Example:**
```c
// Source: src/ecs/ecs_scene.h
static inline bool scene_solver_request_recalculate(ecs_scene_t *scene, ecs_entity_t sketch);
static inline bool scene_solver_can_apply_drag(..., scene_solver_drag_decision_t *out_decision);
static inline bool scene_solver_add_diagnostic(...);
```

### Anti-Patterns to Avoid
- **Variant closure commands per run:** breaks reproducibility and violates D-02.
- **Skipping the rerun after baseline pass:** violates D-03 and hides flakiness.
- **Doc content without code anchors:** fails SDOC-02 and slows debugging.
- **Feature work during closure phase:** violates explicit phase boundary and risks reopening reliability scope.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Test orchestration | Custom shell/test runner scripts | Existing `ctest --test-dir build-vulkan -C Release -R ...` | CTest already knows exact test registration and emits standardized pass/fail summaries. |
| Solver flow explanation | Separate pseudo-architecture divorced from code | Code-anchored mapping from `ecs_scene.h`, `app.c`, `constraint_types.h` | Prevents docs from drifting from real runtime behavior. |
| Flake detection policy | Informal “rerun until pass” behavior | Explicit baseline + immediate rerun failure policy | Determinism is a milestone contract; flaky passes are false confidence. |

**Key insight:** This phase wins by enforcing existing contracts, not by adding infrastructure.

## Common Pitfalls

### Pitfall 1: Regex drift in closure command
**What goes wrong:** Different people run slightly different `-R` expressions and report incompatible results.  
**Why it happens:** Test names are known, so teams improvise command variants.  
**How to avoid:** Lock one canonical regex string in plan/validation/verification artifacts and reuse verbatim.  
**Warning signs:** Verification docs show different test order/name subsets across runs.

### Pitfall 2: Treating “pass once” as deterministic
**What goes wrong:** Baseline pass is accepted without immediate rerun; intermittent failures remain hidden.  
**Why it happens:** Closure pressure and assumption that prior runs are enough.  
**How to avoid:** Make rerun mandatory in task acceptance criteria and phase gate.  
**Warning signs:** Only one command/result block in verification evidence.

### Pitfall 3: Solver docs become theory-only
**What goes wrong:** Documentation is academically useful but not actionable for code debugging.  
**Why it happens:** Literature summary dominates implementation mapping.  
**How to avoid:** Require section-by-section code anchors and a debug-first TL;DR checklist.  
**Warning signs:** No concrete file/function pointers for authoring/recalc/diagnostics/UI.

## Code Examples

Verified patterns from repository sources:

### Canonical 7-test registration (already in build graph)
```cmake
# Source: src/CMakeLists.txt
add_test(NAME script_roundtrip_tests COMMAND script_roundtrip_tests)
add_test(NAME scene_solver_contract COMMAND scene_solver_contract)
add_test(NAME scene_solver_drag COMMAND scene_solver_drag)
add_test(NAME endpoint_pick COMMAND endpoint_pick)
add_test(NAME scene_solver_diagnostics COMMAND scene_solver_diagnostics)
add_test(NAME scene_solver_trigger COMMAND scene_solver_trigger)
add_test(NAME scene_solver_pass_policy COMMAND scene_solver_pass_policy)
```

### Solver lifecycle anchors for docs
```c
// Source: src/ecs/ecs_scene.h
static inline bool scene_solver_request_auto(ecs_scene_t *scene, ecs_entity_t sketch);
static inline void scene_solver_process_auto_queue(ecs_scene_t *scene);
static inline bool scene_solver_request_recalculate(ecs_scene_t *scene, ecs_entity_t sketch);
static inline bool scene_solver_set_max_passes(ecs_scene_t *scene, ecs_entity_t sketch, uint32_t max_passes);
static inline bool scene_solver_set_failure_implication(...);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Ad-hoc phase-close test slices | Locked targeted deterministic rerun gate | Established by prior closure phases (v1.3/v1.4) | Repeatable sign-off, lower ambiguity in milestone closure. |
| Solver knowledge spread across code/tests only | Dedicated solver architecture doc required | Phase 30 scope | Faster onboarding and debugging with stable code anchors. |

**Deprecated/outdated:**
- “Run whichever related tests seem relevant” for closure: replaced by fixed canonical 7-test gate + rerun.
- “Diagnostics as optional detail”: replaced by explicit diagnostics-first failure contracts in tests and verification.

## Open Questions

1. **Exact canonical regex ordering to freeze in Phase 30 artifacts**
   - What we know: D-02 requires one exact string; prior artifacts already use a 7-test regex.
   - What's unclear: Whether to preserve prior regex ordering verbatim or reorder to D-01 list order.
   - Recommendation: Pick one canonical string in Plan Wave 0 and enforce exact reuse everywhere (validation + verification + summaries).

2. **Reference set selection for SDOC-02**
   - What we know: Must include 3-6 high-signal practical references (D-11/D-12).
   - What's unclear: Final curated list was not pre-locked in context.
   - Recommendation: Curate references that directly map to implemented families (coincident/parallel/perpendicular/along/tangency and nonlinear convergence diagnostics), each with “why this helps mdCAD debugging.”

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Windows Vulkan build + test orchestration | ✓ | 4.3.1 | — |
| CTest | Canonical targeted closure gate | ✓ | 4.3.1 | — |
| `build-vulkan` configured tree | Required test-dir for V14 gate | ✓ | existing tree present | Reconfigure with VS generator if stale |
| Vulkan SDK (`VULKAN_SDK`) | Windows Vulkan configure/build path | ✓ | 1.4.341.1 (env var) | — |
| MSVC compiler (`cl`) in current shell | `cmake --build build-vulkan --config Release` | ✗ (not in PATH in this session) | — | Run from Developer PowerShell / VS environment |

**Missing dependencies with no fallback:**
- None (MSVC is expected to be available in proper VS developer environment; this session just lacks `cl` in PATH).

**Missing dependencies with fallback:**
- `cl` in current shell context → use Visual Studio Developer PowerShell or configure CI runner that provides MSVC toolchain.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CTest + native C test executables |
| Config file | `src/CMakeLists.txt` |
| Quick run command | `ctest --test-dir build-vulkan -C Release -R "scene_solver_trigger|scene_solver_pass_policy|scene_solver_diagnostics" --output-on-failure` |
| Full suite command | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| V14-01 | Targeted automated regression coverage across solver/gizmo domains | integration/regression | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure` | ✅ |
| V14-02 | Deterministic Windows Vulkan rerun sign-off | deterministic rerun gate | `cmake --build build-vulkan --config Release` then full command twice | ✅ |
| SDOC-01 | Architecture overview maps authoring→solve→diagnostics→UI | doc contract | `Select-String -Path docs/solver/SOLVER_ARCHITECTURE.md -Pattern "Overview","Solve pipeline","Diagnostics flow","Code anchors","TL;DR"` | ❌ Wave 0 |
| SDOC-02 | Literature + file/function anchors | doc contract | `Select-String -Path docs/solver/SOLVER_ARCHITECTURE.md -Pattern "References","src/ecs/ecs_scene.h","src/app.c","src/constraints/constraint_types.h"` | ❌ Wave 0 |
| SDOC-03 | TL;DR debug primer entry points | doc contract | `Select-String -Path docs/solver/SOLVER_ARCHITECTURE.md -Pattern "unsatisfied","drag rollback","pass-policy"` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `ctest --test-dir build-vulkan -C Release -R "scene_solver_trigger|scene_solver_pass_policy|scene_solver_diagnostics" --output-on-failure`
- **Per wave merge:** canonical 7-test full command
- **Phase gate:** `cmake --build build-vulkan --config Release` + full command baseline + immediate rerun, all green

### Wave 0 Gaps
- [ ] `docs/solver/SOLVER_ARCHITECTURE.md` — required SDOC-01/02/03 artifact
- [ ] Validation checks for solver-doc section and anchor presence (doc-contract checks)

## Sources

### Primary (HIGH confidence)
- `C:\dev\mdCAD\.planning\phases\30-deterministic-closure-gate-windows-vulkan-solver-docs\30-CONTEXT.md` - locked decisions, scope, and deliverables
- `C:\dev\mdCAD\src\CMakeLists.txt` - canonical 7-test CTest registration
- `C:\dev\mdCAD\.planning\REQUIREMENTS.md` - SDOC-01..03 and V14-01..02 requirement contracts
- `C:\dev\mdCAD\.planning\ROADMAP.md` - Phase 30 goals and success criteria
- `C:\dev\mdCAD\docs\QUICKSTART.md` and `C:\dev\mdCAD\docs\VULKAN_WINDOWS.md` - Windows Vulkan build/run guidance
- `C:\dev\mdCAD\src\ecs\ecs_scene.h`, `C:\dev\mdCAD\src\constraints\constraint_types.h`, `C:\dev\mdCAD\src\app.c` - solver and UI flow anchors
- `ctest --test-dir build-vulkan -C Release -N` output - confirms exactly 7 registered tests in configured Vulkan tree
- `C:\dev\mdCAD\.planning\phases\25-regression-and-reliability-closure\25-VALIDATION.md` and `25-VERIFICATION.md` - prior canonical deterministic closure evidence pattern

### Secondary (MEDIUM confidence)
- `C:\dev\mdCAD\.planning\phases\29-active-sketch-line-gizmo-endpoint-authority\29-VALIDATION.md` and `29-VERIFICATION.md` - most recent deterministic rerun evidence posture in v1.4 chain

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** - all recommendations come from existing repository build/test infrastructure and locked context decisions.
- Architecture: **HIGH** - flow anchored to concrete solver/app/constraint symbols in current codebase.
- Pitfalls: **HIGH** - derived from explicit locked failure policy plus prior closure verification artifacts.

**Research date:** 2026-04-09  
**Valid until:** 2026-05-09
