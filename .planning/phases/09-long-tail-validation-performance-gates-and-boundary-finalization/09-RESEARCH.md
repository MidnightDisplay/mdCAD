# Phase 09: Long-Tail Validation, Performance Gates, and Boundary Finalization - Research

**Researched:** 2026-03-28  
**Domain:** Native math harness validation closure, performance-gate evidence, and thin-entrypoint boundary finalization for v1.1 closeout  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Validation and gate posture
- **D-01:** Keep the established harness-first validation posture: strict compare + bench + full `math-validation` workflow as the primary automated gate.
- **D-02:** Preserve the phase-level practical gate style from prior phases (targeted evidence artifacts and runbook alignment), while expanding coverage to satisfy `VAL-01`.
- **D-03:** Treat unresolved gate failures in compare/perf/manual smoke as blocking for Phase 9 completion.

### Performance gate policy
- **D-04:** Continue per-case benchmark evaluation (not aggregate-only), consistent with Phase 5 policy.
- **D-05:** Preserve the no-regression threshold posture already used for native gates (`<= 5%` slowdown per benchmark case, rerun policy for marginal noise bands).
- **D-06:** Ensure both native priority targets are explicitly represented in evidence: macOS Metal and Windows MSVC Vulkan.

### Manual smoke closure scope
- **D-07:** Manual smoke closure must cover serializer, importer, undo/redo, and editor interaction workflows as one end-to-end migration confidence pass.
- **D-08:** Manual evidence should use checklist/report artifacts similar to Phases 6-8 for consistency and auditability.

### Thin-entrypoint boundary finalization
- **D-09:** Remaining thin-entrypoint surface must be intentionally minimal, explicitly documented, and mapped to long-term project-owned boundaries (`TRED-02`).
- **D-10:** Boundary docs should clearly distinguish intentionally retained glue vs removable remnants, with rationale and consumer traceability.

### Scope control
- **D-11:** No new feature capabilities, subsystem rewrites, or platform-expansion work (`PLAT-01`, `PLAT-02`) are in scope for Phase 9.

### the agent's Discretion
- Exact coverage additions for compare/harness cases and evidence formatting, as long as `VAL-01/02/03` and `TRED-02` are demonstrably closed.
- Exact sequencing of automation vs manual workflow evidence capture.
- Exact structure/location of final boundary documentation updates, as long as canonical docs remain consistent.

### Deferred Ideas (OUT OF SCOPE)
- `PLAT-01` iOS validation and `PLAT-02` web/WASM validation remain deferred to future milestone scope.
- Any broad repo-wide `math3d.h` removal beyond intentional thin-entrypoint finalization remains out of scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| TRED-02 | Remaining thin-entrypoint surface is intentionally minimal, documented, and aligned to long-term project-owned boundaries | Existing deferred glue is now concentrated in `src/math3d.h` + harness compatibility (`src/math_harness.c`); boundary finalization should produce explicit retained-vs-removable inventory and consumer traceability evidence. |
| VAL-01 | Compare harness coverage includes long-tail migration touchpoints and passes strict checks on required parity cases | Harness already has 11 strict compare cases across orbit/view/projection/interaction/gizmo/quat/transform/hierarchy; Phase 9 work is to verify long-tail touchpoint coverage map and, if needed, add only missing parity cases. |
| VAL-02 | Expanded migrated math slice shows no native performance regression on macOS Metal and Windows Vulkan benchmark gates | Phase 5 gate machinery is reusable (`eval_math_bench.py`, per-case threshold, rerun policy) and must be rerun for Phase 9 slice with provenance + bench-eval artifacts for both macOS Metal and Windows MSVC Vulkan. |
| VAL-03 | Manual smoke workflows covering serializer/import/undo/editor interactions pass on native macOS and Windows validation paths | Prior phase checklist/report pattern exists and should be consolidated into one integrated end-to-end checklist + report for serializer/import/undo/editor workflows on required native targets. |
</phase_requirements>

## Project Constraints (from copilot-instructions.md)

- Use GSD skill behavior when user explicitly invokes `gsd-*` or `/gsd-*` commands.
- Treat GSD command text as command invocations and align with matching `.github/skills/gsd-*` command definitions.
- Prefer matching custom agents (from `.github/agents`) when command requires subagent behavior.
- Do not apply GSD workflows unless explicitly requested.

## Project Constraints (from AGENTS.md)

- Read `CHECKPOINT.md` at session start for continuity context.
- Keep changes aligned with C-first, header-heavy style (`static inline`, snake_case, explicit error returns).
- Maintain staged migration posture (no one-shot repo-wide legacy helper removal).
- Preserve platform priorities: native macOS Metal and Windows Vulkan are primary confidence gates.

## Summary

Phase 9 is a **closure phase**, not a feature phase. The technical work should focus on proving that the long-tail migrated math surfaces are stable across strict compare parity, benchmark no-regression gates, and integrated manual smoke workflows. The repository already contains mature gate infrastructure: `mdcad_math_harness` compare/bench modes, CMake gate targets (`math-regression`, `math-bench`, `math-validation`), and deterministic performance evaluator policy (`scripts/eval_math_bench.py` with `<=5%` per-case threshold and one-rerun policy for marginal misses).

The harness currently includes 11 compare cases and 9 bench cases, including interaction and undo/editor-sensitive math paths. Phase 9 planning should therefore avoid inventing new frameworks and instead concentrate on (1) explicit coverage mapping to `VAL-01` touchpoints, (2) rerunning native perf evidence for both required targets under the established format, and (3) producing final TRED-02 boundary docs that clearly separate intentional retained compatibility from removable migration remnants.

Thin-entrypoint boundary risk is now concentrated and tractable: deprecated legacy interaction helpers (`ray_from_screen`, `ray_axis_closest_t`, `ray_plane_intersect`) are retained mainly for harness legacy-vs-candidate parity checks; runtime gizmo code already uses `mdcad_interaction_*`. This allows Phase 9 to finalize a minimal retained boundary contract without broad subsystem churn.

**Primary recommendation:** Plan 09-01/09-02/09-03 as **coverage proof → perf gate reruns → integrated smoke + boundary documentation**, reusing existing gate scripts/targets and artifact patterns.

## Standard Stack

### Core
| Library / Tool | Version (verified in repo/toolchain) | Purpose | Why Standard |
|---------|---------|---------|--------------|
| cglm | 0.9.6 (`vendors/cglm/include/cglm/version.h`) | Math backend for migrated slices | Locked milestone backend; existing helper boundaries and harness coverage already target this contract |
| mdcad_math_harness (`src/math_harness.c`) | In-repo target | Strict parity and benchmark execution | Canonical compare/bench source used across Phases 5-8 and required by Phase 9 decisions |
| CMake custom targets (`src/CMakeLists.txt`) | In-repo target wiring | `math-regression`, `math-bench`, `math-validation` gate orchestration | Existing repeatable automation; avoids ad hoc command drift |
| `scripts/eval_math_bench.py` | In-repo script | Deterministic per-case slowdown evaluation and rerun policy handling | Encodes accepted policy (`<=5%`, rerun up to 8% band) and required bench IDs |

### Supporting
| Library / Tool | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Flecs | 4.1.4 (`vendors/flecs/flecs.h`) | ECS runtime touched by transform/hierarchy compare cases | When interpreting transform parity touchpoints in harness |
| cJSON | 1.7.19 (`vendors/cjson/cJSON.h`) | Serializer/import payload parsing | During manual serializer/import smoke coverage |
| cimgui fetch target | `1.92.5dock` (`vendors/libcimgui/CMakeLists.txt`) | UI/editor surfaces used in VAL-03 manual workflows | During integrated editor/undo/manual smoke closure |
| Sokol fetch target | `master` (`vendors/libsokol/CMakeLists.txt`) | Native rendering/runtime execution context for smoke workflows | For platform-specific runtime validation (Metal/Vulkan) |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Existing harness + CMake targets | New external test framework | Out of scope and unnecessary; would add migration risk and break established evidence continuity |
| `scripts/eval_math_bench.py` policy | Manual benchmark interpretation | Inconsistent and error-prone vs codified per-case gate policy |

**Installation:**  
No new package installation is required for Phase 9 planning. Reuse existing repo targets and scripts.

**Version verification approach used here:**  
Version facts were verified from vendored headers/CMake fetch tags and local tool probes (CMake/Python/Node/Vulkan SDK), not training assumptions.

## Architecture Patterns

### Recommended Project Structure
```text
.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/
├── 09-RESEARCH.md
└── evidence/
    ├── coverage/
    │   ├── harness-coverage-map.md
    │   └── compare-strict.txt
    ├── perf/
    │   ├── macos-metal/{baseline,candidate}/...
    │   └── windows-vulkan-msvc/{baseline,candidate}/...
    ├── manual/
    │   ├── long-tail-smoke-checklist.md
    │   └── long-tail-smoke-report.md
    └── boundary/
        └── thin-entrypoint-boundary-finalization.md
```

### Pattern 1: Harness-first strict closure
**What:** Treat `math-validation` + strict compare output as the first blocking gate before manual workflows.  
**When to use:** Every plan wave/task that claims VAL-01 progress.  
**Example:**
```powershell
cmake --build build-vulkan --config Release --target mdcad_math_harness
.\build-vulkan\bin\Release\mdcad_math_harness.exe --mode compare --strict
cmake --build build-vulkan --config Release --target math-validation
```
Source: `src/CMakeLists.txt`, `src/math_harness.c`, `docs/QUICKSTART.md`

### Pattern 2: Deterministic per-case perf gate evidence
**What:** Capture baseline/candidate bench artifacts with provenance and run evaluator script for pass/fail.  
**When to use:** VAL-02 closure on each required target (macOS Metal + Windows MSVC Vulkan).  
**Example:**
```powershell
.\build-vulkan\bin\Release\mdcad_math_harness.exe --mode bench --iterations 2000000 > ...\baseline\bench-run1.txt
.\build-vulkan\bin\Release\mdcad_math_harness.exe --mode bench --iterations 2000000 > ...\candidate\bench-run1.txt
py -3 scripts\eval_math_bench.py --label windows-vulkan-msvc --baseline ... --candidate ... --output ...\bench-eval.md
```
Source: `docs/QUICKSTART.md`, `scripts/eval_math_bench.py`

### Pattern 3: Checklist/report evidence parity with prior phases
**What:** Use targeted checklist + report artifacts mirroring Phases 6/7/8.  
**When to use:** VAL-03 integrated manual closure and TRED-02 boundary documentation proof.  
**Example artifacts:**  
- `serializer-roundtrip-checklist.md` + `serializer-targeted-check-report.md` (Phase 6)  
- `importer-targeted-checklist.md` + `importer-targeted-check-report.md` (Phase 7)  
- `undo-editor-targeted-checklist.md` + `undo-editor-targeted-check-report.md` + `glue-inventory.md` (Phase 8)

### Anti-Patterns to Avoid
- **Reopening architecture decisions:** Phase 9 is not for reselecting stacks or adding new frameworks.
- **Aggregate-only perf pass claims:** Must remain per-case decisioning (`D-04`/`D-05`).
- **Manual-only closure without harness outputs:** Violates harness-first gate posture.
- **Broad repo-wide `math3d` removal:** Explicitly out of scope; boundary finalization must stay minimal and intentional.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Benchmark regression policy | Manual spreadsheet/per-case math | `scripts/eval_math_bench.py` | Already encodes required case set, thresholds, rerun policy, and machine-readable pass/fail |
| Validation orchestration | Custom shell wrappers per operator | CMake targets `math-regression`, `math-bench`, `math-validation` | Existing cross-target command surface reduces drift and reproducibility risk |
| New smoke evidence format | Novel ad hoc template | Existing checklist/report pattern from Phases 6-8 | Maintains auditability and planner/verifier consistency |

**Key insight:** Phase 9 is a confidence/evidence closure phase; custom tooling adds risk without adding requirement coverage.

## Runtime State Inventory

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | None specific to rename/migration key changes in this phase; evidence is file-based under `.planning/phases/*/evidence/` | Code edit only (add Phase 9 evidence files); no data migration needed |
| Live service config | None — verified by scope and repo contents (no external SaaS config artifacts tied to TRED-02/VAL-* closure) | None |
| OS-registered state | None required for boundary finalization; validation runs are command-invoked | None |
| Secrets/env vars | `VULKAN_SDK` required for Windows Vulkan gate execution; no key rename required | Ensure env var present on Windows validation host |
| Build artifacts | Existing build trees (`build`, `build-vulkan`) and harness binaries are reusable; stale binaries may produce misleading evidence if not rebuilt | Rebuild harness/app before each evidence capture (`mdcad_math_harness`, `math-validation`) |

## Common Pitfalls

### Pitfall 1: False VAL-01 closure from existing compare pass
**What goes wrong:** Team assumes current 11 compare cases automatically satisfy long-tail touchpoint closure.  
**Why it happens:** Compare output is green, but explicit requirement-to-case mapping is missing.  
**How to avoid:** Produce a Phase 9 coverage map linking each required long-tail surface (serializer/import/undo/editor/boundary-sensitive math) to compare cases and manual checks.  
**Warning signs:** No artifact that maps `VAL-01` to concrete case IDs and file touchpoints.

### Pitfall 2: Performance claims based on old or low-iteration captures
**What goes wrong:** Perf closure claims reuse superseded or noisy runs.  
**Why it happens:** Existing evidence folders include historical captures; low iteration counts are noisy.  
**How to avoid:** Use high-iteration capture (`2000000`) with fresh provenance and evaluator output per target.  
**Warning signs:** Missing/old provenance timestamps, absent `bench-eval.md`, or mixed iteration counts.

### Pitfall 3: Boundary finalization drifts into broad refactor
**What goes wrong:** TRED-02 task expands into broad legacy helper removal.  
**Why it happens:** `math3d.h` still contains many helpers; temptation to “clean all.”  
**How to avoid:** Limit scope to documented retained minimal boundary + traceable consumer rationale; defer broad removals.  
**Warning signs:** Plan tasks touching unrelated runtime files with no direct TRED-02 closure value.

## Code Examples

### Strict compare and bench listing (source-of-truth discovery)
```powershell
.\build-vulkan\bin\Release\mdcad_math_harness.exe --list
.\build-vulkan\bin\Release\mdcad_math_harness.exe --mode compare --strict
.\build-vulkan\bin\Release\mdcad_math_harness.exe --mode bench --iterations 20000
```
Source: `src/math_harness.c` (`--list`, `--mode`, `--strict`, `--iterations`)

### CMake-native gate runner
```powershell
cmake --build build-vulkan --config Release --target mdcad_math_harness
cmake --build build-vulkan --config Release --target math-regression
cmake --build build-vulkan --config Release --target math-bench
cmake --build build-vulkan --config Release --target math-validation
```
Source: `src/CMakeLists.txt`, `docs/QUICKSTART.md`

### Boundary consumer inventory command
```powershell
Get-ChildItem -Path src -Recurse -Include *.h,*.c |
  Select-String -Pattern 'ray_from_screen\(|ray_axis_closest_t\(|ray_plane_intersect\('
```
Source: Phase 9 TRED-02 boundary finalization need; validates retained legacy-helper consumer set.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Light phase-local compile + targeted checks only | Harness-first strict + bench + full `math-validation` + targeted evidence | Established by Phase 5 policy, carried through Phase 9 decisions | Provides stronger automated confidence and consistent closure criteria |
| Implicit glue retention | Explicit retained/deferred glue inventory with traceability | Phase 8 | Enables TRED-02 minimal-boundary finalization without destabilizing runtime |

**Deprecated/outdated for this phase:**
- New bespoke validation framework introduction — unnecessary and out-of-scope.
- Aggregate-only benchmark claims — replaced by per-case threshold evaluation.

## Open Questions

1. **Do compare-case additions need to include serializer/import-specific math checks, or is explicit mapping to existing coverage sufficient?**
   - What we know: Existing compare set strongly covers interaction/transform/quaternion/hierarchy paths.
   - What's unclear: Exact required breadth for “long-tail migration touchpoints” wording in `VAL-01`.
   - Recommendation: Make 09-01 first deliverable a coverage map artifact; add cases only for unmapped required touchpoints.

2. **Which host executes macOS Metal perf/manual evidence if current environment is Windows?**
   - What we know: Current probe environment has Windows Vulkan toolchain; no local macOS runtime.
   - What's unclear: Whether cross-host evidence capture is pre-arranged for Phase 9.
   - Recommendation: Plan explicit human/CI handoff for macOS evidence capture with same artifact schema and provenance format.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | All gate/build orchestration | ✓ | 4.3.0 | — |
| Python | `scripts/eval_math_bench.py` | ✓ | 3.13.x (`python`, `py -3`) | — |
| Node.js | GSD/init tooling | ✓ | v25.8.1 | — |
| Git | provenance capture and workflow continuity | ✓ | 2.51.1.windows.1 | — |
| Vulkan SDK (`VULKAN_SDK`) | Windows MSVC Vulkan validation path | ✓ | 1.4.341.1 (env var set) | No fallback for Vulkan hard gate |
| VS 2026 toolchain via CMake generator | Windows MSVC gate target | ✓ | MSVC 19.50.x detected by CMake | — |
| Ninja CLI | macOS Ninja command path in docs | ✗ (current host) | — | Use Visual Studio generator on current host; macOS evidence must be captured on macOS host |

**Missing dependencies with no fallback:**
- None blocking Windows-side Phase 9 research/planning.

**Missing dependencies with fallback:**
- Local Ninja not present on this Windows host; use `cmake --build` with VS generator locally, and treat macOS Ninja runs as external-host evidence task.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | In-repo native harness (`mdcad_math_harness`) + CMake custom targets |
| Config file | none — target wiring in `src/CMakeLists.txt` |
| Quick run command | `cmake --build build-vulkan --config Release --target mdcad_math_harness` |
| Full suite command | `cmake --build build-vulkan --config Release --target math-validation` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| TRED-02 | Minimal retained thin-entrypoint boundary is explicit and justified | static audit + evidence doc | symbol inventory grep + boundary report update | ❌ Wave 0 (new Phase 9 artifact) |
| VAL-01 | Strict compare parity covers long-tail touchpoints | regression compare | `.\build-vulkan\bin\Release\mdcad_math_harness.exe --mode compare --strict` | ✅ |
| VAL-02 | No per-case benchmark regression on required native targets | benchmark gate | harness bench captures + `py -3 scripts\eval_math_bench.py ...` | ✅ |
| VAL-03 | Integrated manual smoke across serializer/import/undo/editor | manual integration checklist | app run + checklist/report completion | ❌ Wave 0 (new Phase 9 checklist/report) |

### Sampling Rate
- **Per task commit:** `cmake --build build-vulkan --config Release --target mdcad_math_harness` + strict compare
- **Per wave merge:** `cmake --build build-vulkan --config Release --target math-validation`
- **Phase gate:** Full required artifacts green for both native targets before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/coverage/harness-coverage-map.md` — maps VAL-01 touchpoints to compare/manual evidence
- [ ] `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/manual/long-tail-smoke-checklist.md` — integrated VAL-03 checklist
- [ ] `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/manual/long-tail-smoke-report.md` — integrated VAL-03 report
- [ ] `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/boundary/thin-entrypoint-boundary-finalization.md` — TRED-02 retained-vs-removable boundary contract

## Sources

### Primary (HIGH confidence)
- `C:\dev\mdCAD\.planning\phases\09-long-tail-validation-performance-gates-and-boundary-finalization\09-CONTEXT.md` — locked decisions and scope constraints
- `C:\dev\mdCAD\.planning\REQUIREMENTS.md` — TRED-02/VAL-01/VAL-02/VAL-03 requirement definitions
- `C:\dev\mdCAD\.planning\ROADMAP.md` — Phase 9 plan split (`09-01`..`09-03`) and success criteria
- `C:\dev\mdCAD\src\math_harness.c` — compare/bench case inventory, CLI options, strict mode behavior
- `C:\dev\mdCAD\src\CMakeLists.txt` — harness target wiring (`mdcad_math_harness`, `math-regression`, `math-bench`, `math-validation`)
- `C:\dev\mdCAD\scripts\eval_math_bench.py` — benchmark gate threshold and rerun policy logic
- `C:\dev\mdCAD\docs\QUICKSTART.md` — canonical command sequences for native validation and perf captures
- `C:\dev\mdCAD\.planning\phases\08-undo-editor-utility-migration-and-glue-burn-down\evidence\glue-inventory.md` — deferred-to-Phase-9 boundary carry-over
- `C:\dev\mdCAD\.planning\phases\08-undo-editor-utility-migration-and-glue-burn-down\evidence\undo-editor-targeted-check-report.md` — Phase 8 deferred glue rationale
- `C:\dev\mdCAD\src\math3d.h`, `src\math\math_interaction.h`, `src\gizmo\gizmo.h` — current retained legacy helper boundary vs runtime migrated consumers

### Secondary (MEDIUM confidence)
- `C:\dev\mdCAD\AGENTS.md` — project conventions and staged migration constraints
- `C:\dev\mdCAD\.github\copilot-instructions.md` — GSD command/tooling behavior constraints

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** — verified directly from repository source/config and local tool probes
- Architecture: **HIGH** — based on concrete existing harness/target/evidence patterns in Phases 5-8
- Pitfalls: **HIGH** — derived from known phase carry-over artifacts and explicit requirement/gate policy constraints

**Research date:** 2026-03-28  
**Valid until:** 2026-04-27 (30 days; repo-local process/tooling assumptions)
