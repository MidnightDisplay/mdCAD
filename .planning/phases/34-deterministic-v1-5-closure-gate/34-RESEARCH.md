# Phase 34: Deterministic v1.5 Closure Gate - Research

**Researched:** 2026-04-11  
**Domain:** Deterministic closure-gate policy and reproducible sign-off evidence (Windows Vulkan, CTest)  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
## Implementation Decisions

### Closure gate command contract and platform scope
- **D-01:** Reuse Phase 30 closure contract exactly for v1.5 sign-off.
- **D-02:** Windows Vulkan remains the only required closure platform scope for this phase.
- **D-03:** Use one canonical 7-test gate command string (no command-format variants for closure evidence).
- **D-04:** Deterministic sign-off requires baseline pass plus immediate rerun pass using the identical canonical command.

### Evidence strictness policy
- **D-05:** Verification artifacts must include the exact command and baseline/rerun result summaries.
- **D-06:** Raw logs are not mandatory artifacts for closure; include only when needed for debugging.

### Divergence and flake handling
- **D-07:** If baseline and immediate rerun diverge for any reason (including flake), closure is a hard fail.
- **D-08:** Stabilization is required before sign-off can proceed.

### Historical warning-marker handling
- **D-09:** Historical human-UAT warning markers in lifecycle docs are treated as non-blocking metadata once `DIAG-03` deterministic gate requirements are satisfied.

### the agent's Discretion
- Exact wording/layout of verification summaries, while preserving D-05 and D-06.
- Exact location/shape of optional debug log excerpts when closure investigation is needed.

### Deferred Ideas (OUT OF SCOPE)
## Deferred Ideas

None — discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| DIAG-03 | Developer can verify deterministic baseline + immediate rerun parity on the v1.5 targeted regression gate. | Reuse locked Phase 30 canonical 7-test command and enforce baseline + immediate rerun parity with hard-fail divergence policy in Phase 34 validation/verification artifacts. |
</phase_requirements>

## Summary

Phase 34 is a **contract-preservation closure phase**, not an implementation-feature phase. The key planning move is to preserve Phase 30 deterministic closure semantics exactly: same Windows Vulkan scope, same canonical 7-test command, same baseline+immediate-rerun requirement, and same hard-fail-on-divergence policy.

The code/test infrastructure needed for DIAG-03 already exists and is verifiable in this repo (`src/CMakeLists.txt` includes all seven tests; `build-vulkan` contains all seven test registrations). Risk is process drift (command-string variants, missing rerun, or acceptance of flaky divergence), not missing solver functionality.

**Primary recommendation:** Plan Phase 34 as deterministic evidence production only: run/build with the locked canonical command contract, record baseline + immediate rerun summaries, and fail closure on any divergence until stabilized.

## Project Constraints (from copilot-instructions.md)

`./copilot-instructions.md` is not present. No additional directives were discovered from that file.

## Standard Stack

### Core
| Library/Tool | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| CMake | 4.3.1 (verified locally) | Build invocation for closure gate precondition | Existing project-native build workflow and locked closure sequence require it. |
| CTest | 4.3.1 (verified locally) | Deterministic targeted gate execution | Canonical 7-test gate is already registered and used in prior closure phases. |
| Windows Vulkan build tree (`build-vulkan`) | existing configured tree | Required closure test directory/scope | Locked by D-01/D-02 and prior phase contract. |

### Supporting
| Library/Tool | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Vulkan SDK env (`VULKAN_SDK`) | 1.4.341.1 (env var) | Windows Vulkan toolchain presence | Required when reconfiguring/rebuilding Vulkan target in closure workflow. |
| Existing targeted tests | repo-local | Regression surface for DIAG-03 | Use exactly the canonical seven tests for baseline and immediate rerun. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Canonical locked 7-test command | Variant command forms / reordered regex | Violates D-03 and weakens reproducibility evidence. |
| Windows Vulkan-only closure scope | Add macOS/Linux closure requirements | Explicitly out of scope per D-02 and phase boundary. |
| Summary-first evidence artifact | Full mandatory raw logs | Conflicts with D-06 (logs optional unless debugging). |

## Architecture Patterns

### Recommended Project Structure
```text
.planning/phases/34-deterministic-v1-5-closure-gate/
├── 34-CONTEXT.md
├── 34-RESEARCH.md
├── 34-VALIDATION.md      # to define Nyquist test contract for DIAG-03
└── 34-VERIFICATION.md    # to record build + baseline + rerun evidence
```

### Pattern 1: Locked deterministic closure command contract
**What:** Reuse exact Phase 30 command string and run order.  
**When to use:** Every Phase 34 closure verification run and artifact capture.  
**Example:**
```powershell
# Source: .planning/phases/30-deterministic-closure-gate-windows-vulkan-solver-docs/30-VALIDATION.md
cmake --build build-vulkan --config Release
ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure
ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure
```

### Pattern 2: Summary-first closure evidence with optional debug appendix
**What:** Capture exact command + concise baseline/rerun result summaries; attach raw logs only if diagnosing a failure.  
**When to use:** `34-VERIFICATION.md` authoring and closure sign-off review.

### Anti-Patterns to Avoid
- **Command drift:** Any regex/format variant creates non-comparable evidence.
- **Single-run acceptance:** Baseline-only pass does not satisfy DIAG-03.
- **Flake forgiveness:** Any baseline/rerun divergence must fail closure (D-07).
- **Historical warning marker as blocker:** D-09 says these become non-blocking metadata once DIAG-03 passes.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Gate orchestration | Custom scripts that duplicate test selection logic | Existing canonical `ctest --test-dir build-vulkan -C Release -R ...` command | Registered suite is already authoritative and deterministic. |
| Determinism proof format | New ad-hoc closure criteria | Existing baseline + immediate rerun parity contract | Already accepted in prior closure phase and aligned with DIAG-03. |
| Flake policy | Retry-until-green heuristics | Hard fail on divergence, stabilize first | Preserves reliability claim credibility. |

**Key insight:** This phase should maximize **policy continuity**, not introduce new mechanics.

## Common Pitfalls

### Pitfall 1: Canonical command mismatch between documents
**What goes wrong:** Validation and verification artifacts use slightly different regex strings.  
**Why it happens:** Manual copy/edit drift.  
**How to avoid:** Paste one locked string verbatim into all Phase 34 artifacts.  
**Warning signs:** Different test counts/order between baseline/rerun evidence blocks.

### Pitfall 2: Treating rerun divergence as “acceptable flake”
**What goes wrong:** Closure proceeds despite mismatch.  
**Why it happens:** Pressure to close milestone quickly.  
**How to avoid:** Explicit fail-fast acceptance criteria in plan and verification checklist.  
**Warning signs:** Notes like “passed on second retry” in closure artifact.

### Pitfall 3: Over-collecting logs and obscuring decision signal
**What goes wrong:** Artifact bloats with raw output while summary contract is unclear.  
**Why it happens:** Teams equate more text with stronger evidence.  
**How to avoid:** Keep command+summary mandatory; raw logs optional appendix only.  
**Warning signs:** Closure conclusion hard to derive without parsing long logs.

## Code Examples

Verified patterns from repository sources:

### Canonical seven tests are registered
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

### Gate membership verification command
```powershell
# Source: repository CTest usage pattern + local probe
ctest --test-dir build-vulkan -C Release -N -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests"
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Closure acceptance by broad “related tests” confidence | Locked canonical targeted gate + immediate rerun parity | v1.4 closure policy (Phase 30) | Deterministic, auditable sign-off with less ambiguity. |
| Warning markers could implicitly block closure | DIAG-03 deterministic evidence governs closure; legacy markers become metadata | Phase 34 context decision D-09 | Prevents historical marker noise from overriding deterministic gate truth. |

**Deprecated/outdated:**
- Any closure policy that allows command variants or flaky rerun acceptance for this phase.

## Open Questions

1. **Should Phase 34 execute fresh closure runs in current environment or document contract-only if MSVC shell is unavailable?**
   - What we know: `cl` is not present in current shell; `VULKAN_SDK` exists; `build-vulkan` and seven tests are present.
   - What's unclear: Whether execution will occur in a Developer PowerShell context during phase implementation.
   - Recommendation: Plan explicit environment preflight and fallback instruction: run closure commands in MSVC-enabled shell.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Closure build pre-step | ✓ | 4.3.1 | — |
| CTest | Canonical deterministic gate runs | ✓ | 4.3.1 | — |
| `build-vulkan` test tree | Required `--test-dir` target | ✓ | existing | Reconfigure if stale |
| Vulkan SDK env | Windows Vulkan toolchain | ✓ | `1.4.341.1` (env) | — |
| MSVC compiler (`cl`) in current shell | `cmake --build build-vulkan --config Release` in this shell | ✗ | — | Run in Visual Studio Developer PowerShell |

**Missing dependencies with no fallback:**
- None.

**Missing dependencies with fallback:**
- `cl` missing in current shell context; use MSVC-enabled Developer PowerShell/session.

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
| DIAG-03 | Deterministic baseline + immediate rerun parity for v1.5 targeted gate | deterministic rerun gate | `cmake --build build-vulkan --config Release` then run full suite command twice consecutively | ✅ |

### Sampling Rate
- **Per task commit:** quick run command
- **Per wave merge:** full suite command once
- **Phase gate:** build + full suite baseline + immediate rerun; all must match pass outcome

### Wave 0 Gaps
- [ ] `.planning/phases/34-deterministic-v1-5-closure-gate/34-VALIDATION.md` — lock DIAG-03 contract and command string.
- [ ] `.planning/phases/34-deterministic-v1-5-closure-gate/34-VERIFICATION.md` — capture build + baseline + immediate rerun summaries.

## Sources

### Primary (HIGH confidence)
- `.planning/phases/34-deterministic-v1-5-closure-gate/34-CONTEXT.md` — locked decisions D-01..D-09 and scope.
- `.planning/REQUIREMENTS.md` — DIAG-03 contract text and traceability.
- `.planning/ROADMAP.md` — Phase 34 goal/success criteria.
- `.planning/phases/30-deterministic-closure-gate-windows-vulkan-solver-docs/30-CONTEXT.md` — canonical prior closure contract baseline.
- `.planning/phases/30-deterministic-closure-gate-windows-vulkan-solver-docs/30-VALIDATION.md` — canonical command and deterministic sequence.
- `src/CMakeLists.txt` — registered targeted seven tests.
- `docs/VULKAN_WINDOWS.md` — Windows Vulkan workflow guidance.
- Local environment probes (`cmake --version`, `ctest --version`, `ctest -N -R ...`) — tool and test availability.

### Secondary (MEDIUM confidence)
- `.planning/phases/33-large-jump-robustness-and-parallel-along-parity/33-VERIFICATION.md` — upstream deterministic posture continuity.
- `.planning/phases/33-large-jump-robustness-and-parallel-along-parity/33-HUMAN-UAT.md` — historical warning-marker context relevant to D-09.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** — directly verified from repository and local tool/runtime checks.
- Architecture: **HIGH** — constrained by explicit locked user decisions and established Phase 30 contract.
- Pitfalls: **HIGH** — derived from explicit divergence policy and prior closure evidence patterns.

**Research date:** 2026-04-11  
**Valid until:** 2026-05-11
