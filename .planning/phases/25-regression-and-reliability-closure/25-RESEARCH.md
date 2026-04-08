# Phase 25: Regression and Reliability Closure - Research

**Researched:** 2026-04-08  
**Domain:** Native C/CMake regression gate closure for solver reliability (Windows Vulkan targeted)  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Regression matrix scope
- **D-01:** Baseline closure gate uses the current targeted solver+script suite (do not expand to full `ctest` for this phase).
- **D-02:** Baseline gate remains exactly these 7 tests:
  - `script_roundtrip_tests`
  - `scene_solver_contract`
  - `scene_solver_pass_policy`
  - `scene_solver_diagnostics`
  - `scene_solver_trigger`
  - `scene_solver_drag`
  - `endpoint_pick`
- **D-03:** Do not add extra sentinels (for example `mdCAD` build target) in Phase 25 scope unless needed to fix a discovered regression.

### Failure policy
- **D-04:** Strict pass/fail closure: any failing test blocks Phase 25 closure.
- **D-05:** If baseline gate fails, fix within Phase 25 before closure (no defer-as-known-fail policy).
- **D-06:** Flaky behavior is treated as failure; stabilize or rewrite to deterministic behavior before closure.
- **D-07:** Explicit per-family diagnostics behavior remains mandatory closure contract, not optional metadata.

### Evidence and provenance expectations
- **D-08:** Mandatory evidence is command + result summary in verification artifacts (raw output files are not required by default).
- **D-09:** Closure requires a fresh rerun at closure time, even if earlier phase runs were green.
- **D-10:** Verification posture is automation-first; manual checks are only required when a discovered regression specifically needs manual confirmation.

### Scope guardrails
- **D-11:** No new constraint features in Phase 25; regression closure only.
- **D-12:** New feature ideas discovered during Phase 25 are recorded as deferred/backlog items, not folded into execution scope.
- **D-13:** Closure remains on established Windows Vulkan gate scope for this phase (no cross-platform expansion in Phase 25).

### the agent's Discretion
- Exact test orchestration mechanics (single command vs staged command blocks), as long as D-01..D-13 remain satisfied.
- Exact wording/format for summary evidence in verification artifacts, as long as command + result is explicit and reproducible.

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| V13-01 | Developers can run automated regression coverage for trigger integrity, iterative pass behavior, and new constraint legality/solve semantics. | Confirms canonical 7-test CTest gate exists in `src/CMakeLists.txt`, maps each test binary to coverage area, and defines strict rerun closure workflow + evidence contract for deterministic pass/fail. |
</phase_requirements>

## Summary

Phase 25 is a **closure phase**, not an implementation-expansion phase. The required gate is already concretely encoded in the repository as exactly 7 CTest targets and matches the locked decision set. Planning should focus on deterministic execution, strict failure handling, and verification artifact quality—not adding tests/features/tools unless a discovered regression forces a scoped fix.

The current codebase already embeds coverage for the three required reliability surfaces: trigger integrity (`scene_solver_trigger`), iterative pass policy (`scene_solver_pass_policy`), and legality/solve semantics for directional and ARCI families (`scene_solver_contract`, `scene_solver_diagnostics`, `endpoint_pick`), plus script/runtime stability (`script_roundtrip_tests`) and drag integration (`scene_solver_drag`). This means Phase 25 should primarily orchestrate, run, diagnose, and close with fresh evidence.

**Primary recommendation:** Implement Phase 25 as a two-wave closure loop: (1) baseline gate execution + triage, (2) mandatory fresh rerun + verification artifact finalization, with strict stop-the-line on any failure/flakiness.

## Project Constraints (from copilot-instructions.md)

`./copilot-instructions.md` was not found in repository root at research time; no additional project-specific directives were discovered from that file.

## Standard Stack

### Core
| Library/Tool | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| CMake | 4.3.0 (installed) | Build orchestration and test registration | Canonical project build/test entrypoint; all 7 gate binaries are registered here. |
| CTest | 4.3.0 (installed) | Deterministic execution of named regression targets | Enables exact-name gate enforcement and reproducible pass/fail summaries. |
| Existing 7 native test binaries | repo-defined | Coverage for V13-01 reliability contract | Already aligned to roadmap/context decisions; no new framework required. |

### Supporting
| Library/Tool | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Git | 2.51.1.windows.1 (installed) | Provenance anchoring for closure evidence | Always include commit SHA in verification artifact summaries. |
| `build-vulkan` configured test tree | local build state | Windows Vulkan-scoped closure target environment | Use for all Phase 25 closure runs per D-13. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| 7-test targeted gate | full `ctest` suite | Violates locked D-01/D-02 scope for this phase; use only if regression diagnosis requires exploratory checks. |

**Installation:** Not applicable (uses in-repo native build/test stack already present).

## Architecture Patterns

### Recommended Project Structure
```
.planning/phases/25-regression-and-reliability-closure/
├── 25-CONTEXT.md        # Locked scope and closure policy
├── 25-PLAN.md           # Execution tasks (or multiple PLANs)
├── 25-VALIDATION.md     # Requirement-to-test mapping
└── 25-VERIFICATION.md   # Command + result summaries, fresh rerun evidence
```

### Pattern 1: Targeted Named CTest Gate (Exact 7)
**What:** Execute only the locked 7 tests via regex/name-filtered CTest command.  
**When to use:** Every Phase 25 verification run (baseline and final rerun).  
**Example:**
```bash
ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure
```
Source: `src/CMakeLists.txt`, `.planning/phases/25-regression-and-reliability-closure/25-CONTEXT.md`

### Pattern 2: Strict Closure Loop
**What:** Run gate → if any fail, fix immediately in-scope → rerun until green → do mandatory fresh rerun for closure evidence.  
**When to use:** Always (D-04..D-06, D-09).  

### Anti-Patterns to Avoid
- **Scope creep during failures:** Do not add new feature work while fixing regressions (D-11/D-12).
- **“Known fail” acceptance:** Any fail or flake blocks closure (D-04..D-06).
- **Stale evidence reuse:** Earlier green runs are insufficient; closure requires fresh rerun (D-09).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Test orchestration | Custom ad-hoc shell harnesses | CTest with named tests | Existing registration is canonical and auditable. |
| Gate composition | New sentinel tests unrelated to V13-01 | Locked 7-test baseline | Scope is fixed by context decisions. |
| Failure bookkeeping | Informal notes only | Verification artifact with command + result summary | Required closure provenance (D-08). |

**Key insight:** Phase 25 succeeds by disciplined execution of existing assets, not by adding infrastructure.

## Common Pitfalls

### Pitfall 1: Accidental gate expansion
**What goes wrong:** Planner broadens to full suite or adds extra sentinels “for safety.”  
**Why it happens:** Good intent but ignores locked D-01..D-03.  
**How to avoid:** Hard-code exact 7-test command in all Phase 25 plan tasks.  
**Warning signs:** Command lacks `-R` filter or includes additional targets.

### Pitfall 2: Treating flaky as acceptable
**What goes wrong:** Intermittent pass accepted as “probably fine.”  
**Why it happens:** Closure pressure.  
**How to avoid:** Enforce deterministic reruns; any flake is failure (D-06).  
**Warning signs:** Inconsistent outcomes across repeated identical runs.

### Pitfall 3: Closure without fresh rerun
**What goes wrong:** Team closes based on earlier phase evidence.  
**Why it happens:** Prior green run exists (e.g., Phase 24 verification).  
**How to avoid:** Make final task a mandatory fresh command execution (D-09).  
**Warning signs:** Verification artifact timestamps precede final code changes.

## Code Examples

Verified patterns from repository sources:

### Enumerate exact gate in configured build
```bash
ctest -N -C Release --test-dir build-vulkan
```
Expected tests:
- script_roundtrip_tests
- scene_solver_contract
- scene_solver_drag
- endpoint_pick
- scene_solver_diagnostics
- scene_solver_trigger
- scene_solver_pass_policy

Source: local `ctest -N` output, `src/CMakeLists.txt`

### Execute strict closure gate
```bash
ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure
```
Source: Phase 24 verification precedent + `src/CMakeLists.txt` registered tests

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Broad/manual confidence checks | Focused deterministic reliability gate | v1.3 phases 22-24 leading into 25 | Faster, auditable closure tied directly to solver reliability requirements. |
| Implicit or generic failures | Explicit family diagnostics + implication contracts | Phases 22-24 | Enables strict closure criteria for semantics and unsat behavior. |

**Deprecated/outdated:**
- “Known-fail defer for closure” posture is explicitly out-of-policy for this phase.

## Open Questions

1. **If failures appear, where should fix scope stop?**
   - What we know: D-11/D-12 prohibit feature expansion; only regression fixes are in scope.
   - What's unclear: Exact threshold for “minimal fix” vs. structural refactor.
   - Recommendation: Require each fix task to cite failing test + exact contract line it restores.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Build/test orchestration | ✓ | 4.3.0 | — |
| CTest | Gate execution | ✓ | 4.3.0 | — |
| Git | Evidence provenance (commit anchoring) | ✓ | 2.51.1.windows.1 | — |
| `build-vulkan` configured test tree | Windows Vulkan gate (D-13) | ✓ | local configured | Reconfigure build tree if invalid |

**Missing dependencies with no fallback:**
- None.

**Missing dependencies with fallback:**
- None.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CTest (CMake-integrated native C test executables) |
| Config file | CMake/CTest metadata in `build-vulkan/CTestTestfile.cmake` (generated from `src/CMakeLists.txt`) |
| Quick run command | `ctest --test-dir build-vulkan -C Release -R "scene_solver_trigger|scene_solver_pass_policy|scene_solver_diagnostics" --output-on-failure` |
| Full suite command | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| V13-01 | Auto-solve trigger integrity | unit/integration-native | `ctest --test-dir build-vulkan -C Release -R "scene_solver_trigger" --output-on-failure` | ✅ |
| V13-01 | Iterative pass tolerance + pass-cap policy | unit/integration-native | `ctest --test-dir build-vulkan -C Release -R "scene_solver_pass_policy" --output-on-failure` | ✅ |
| V13-01 | Legality + deterministic solve semantics (AXIS/ARCI + diagnostics) | unit/integration-native | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_diagnostics|endpoint_pick" --output-on-failure` | ✅ |
| V13-01 | Script/runtime roundtrip stability anchor in gate | regression-native | `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests" --output-on-failure` | ✅ |
| V13-01 | Drag + solver integration reliability anchor | regression-native | `ctest --test-dir build-vulkan -C Release -R "scene_solver_drag" --output-on-failure` | ✅ |

### Sampling Rate
- **Per task commit:** quick run command
- **Per wave merge:** full 7-test command
- **Phase gate:** full 7-test command green + mandatory fresh rerun green before `/gsd-verify-work`

### Wave 0 Gaps
None — existing test infrastructure already covers Phase 25 requirement scope and exact locked baseline.

## Sources

### Primary (HIGH confidence)
- `.planning/phases/25-regression-and-reliability-closure/25-CONTEXT.md` - locked closure decisions D-01..D-13 and evidence policy.
- `.planning/REQUIREMENTS.md` - V13-01 contract definition and traceability.
- `.planning/ROADMAP.md` - Phase 25 goal/success criteria.
- `src/CMakeLists.txt` - canonical registration of the exact 7 CTest targets.
- `src/tests/scene_solver_trigger_test.c` - trigger queue/coalescing/manual override coverage.
- `src/tests/scene_solver_pass_policy_test.c` - tolerance/pass-cap explicit diagnostics coverage.
- `src/tests/scene_solver_contract_test.c` - deterministic transactional solver semantics including ALONG/ARCI contracts.
- `src/tests/scene_solver_diagnostics_test.c` - explicit per-family diagnostics behavior.
- `src/tests/scene_solver_drag_test.c` - drag integration reliability checks.
- `src/tests/endpoint_pick_test.c` - legality/participant role and endpoint semantics checks.
- `src/tests/script_roundtrip_tests.c` - script apply/emit/runtime stability anchor.
- Local environment probe (`cmake --version`, `ctest --version`, `ctest -N -C Release --test-dir build-vulkan`) - availability and concrete gate enumeration.

### Secondary (MEDIUM confidence)
- `.planning/phases/24-advanced-arc-line-arc-constraint-expansion/24-VERIFICATION.md` - prior use of identical 7-test closure command pattern.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - derived from concrete in-repo build/test wiring and local tool versions.
- Architecture: HIGH - directly constrained by locked context decisions and existing phase artifacts.
- Pitfalls: HIGH - directly inferred from explicit failure/scope/evidence policies in Phase 25 context.

**Research date:** 2026-04-08  
**Valid until:** 2026-05-08 (stable, repository-internal workflow)

