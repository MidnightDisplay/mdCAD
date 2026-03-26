# Phase 5: Windows Vulkan Hardening and Performance Gates - Research

**Researched:** 2026-03-25
**Domain:** Windows Vulkan runtime hardening plus native benchmark gates for migrated math hotspots
**Confidence:** HIGH

## User Constraints (from 05-CONTEXT.md)

### Locked Decisions and Non-Negotiables
- `MSVC + Vulkan` is the only Windows hard release gate; `MinGW + Vulkan` is smoke-only.
- Windows manual smoke must include: Phase 4 interaction checklist plus scene navigation and import/save sanity.
- Evidence bundle is mandatory: command log + harness outputs + concise checklist summary.
- Phase 5 cannot close until Windows Vulkan hard gate is green.
- Performance pass/fail is per bench case, per target, with `<= 5%` slowdown tolerance.
- If a case narrowly misses, allow one rerun and use the better (lower `avg_ns`) result.
- All current harness bench cases are gating.
- Reproducible Vulkan-only pick/readback hitching is in scope and must be fixed or safely mitigated.
- If full fix is too large, land smallest safe mitigation with measured residual risk; phase remains open if gates still fail.
- Non-Windows failures are collateral unless they impact `HOT-04`, `PERF-02`, `PERF-03`.
- Backlog handoff must use structured table (`area`, `issue`, `evidence`, `impact`, `recommended phase`) with priority tiers `P1/P2/P3`.

### Requirement IDs This Phase Must Close
- `HOT-04`: migrated hotspots build and behave correctly on Windows Vulkan.
- `PERF-02`: no native performance regression on macOS Metal.
- `PERF-03`: no native performance regression on Windows Vulkan.

## Execution Constraints and Gates

### Hard Execution Constraints
- Keep scope bounded to hardening + validation + evidence + backlog handoff. No new runtime feature scope.
- Keep backend routing unchanged (`src/platform.h` and `src/CMakeLists.txt` already define Windows Vulkan selection via `USE_VULKAN` / MinGW).
- Keep harness-first validation posture (`math-regression`, `math-bench`, `math-validation`) and only treat app smoke as behavioral confirmation.
- Preserve current benchmark catalog in `src/math_harness.c` as the required gate set.

### Exit Gates (Phase-Level)
1. `HOT-04` gate:
   - `build-vulkan` compiles and runs `mdcad_math_harness`, strict compare passes, and manual Windows Vulkan smoke checklist passes.
2. `PERF-02` gate:
   - macOS benchmark evidence shows every bench case within `<= 5%` slowdown vs macOS baseline.
3. `PERF-03` gate:
   - Windows Vulkan benchmark evidence shows every bench case within `<= 5%` slowdown vs Windows Vulkan baseline.
4. Evidence gate:
   - Both platforms have archived command logs + raw harness outputs + interpreted results + checklist summaries.
5. Handoff gate:
   - Remaining long-tail/platform/residual hardening work captured in structured backlog table with priority.

## Current Technical Reality (Planning-Relevant)

### Existing Strengths You Can Plan Around
- `src/CMakeLists.txt` already exposes `mdcad_math_harness`, `math-regression`, `math-bench`, `math-validation` for native desktop.
- `src/math_harness.c` already emits parseable lines:
  - compare: `COMPARE PASS|FAIL ...`
  - bench: `BENCH <name> iterations=... elapsed_ms=... avg_ns=...`
- `docs/QUICKSTART.md` already contains macOS and Windows Vulkan command paths.
- `src/app.c` already gates pick rebuild frequency (`pick_buffer_needs_rebuild`) based on cursor movement + camera hash.

### Known Vulkan Hotspot
`src/gpu/pick_readback_vulkan.c` currently does per-readback command/resource work and blocks on `vkQueueWaitIdle(queue)`.
This is correctness-oriented but can hitch interaction under frequent pick rebuilds.

## Implementation Options and Tradeoffs

### Option A: Keep current Vulkan readback design and only tune rebuild cadence
- Changes: tighten rebuild heuristics in `pick_buffer.h` only.
- Pros: smallest code change; low regression risk.
- Cons: does not remove queue-wide stall; likely insufficient if hitch is reproducible under required workflows.
- Fit: only acceptable if measured hitch is already below user-visible threshold and all gates pass.

### Option B: Vulkan readback hardening with persistent resources + fence sync (recommended)
- Changes:
  - Keep a persistent command pool/buffer and staging buffer (resize only when needed).
  - Replace `vkQueueWaitIdle` with per-submit fence wait (`vkWaitForFences`) to avoid stalling unrelated queue work.
  - Keep current image layout transitions and correctness semantics.
- Pros: meaningful hitch reduction with moderate complexity; preserves synchronous pick semantics.
- Cons: more lifecycle/state handling; requires careful cleanup and failure paths.
- Fit: best balance for Phase 5 bounded scope.

### Option C: Async pick readback pipeline (N-frame latency)
- Changes: queue copy work and consume pick result in later frame(s).
- Pros: highest potential responsiveness gain.
- Cons: behavior change (hover/click latency), larger scope, higher validation burden, risk of Phase 5 overrun.
- Fit: better as follow-up unless Option B cannot satisfy gates.

### Performance Gate Implementation Options
- Manual comparison in docs/spreadsheet:
  - Pros: minimal engineering.
  - Cons: error-prone and slower to audit.
- Lightweight parser script for `BENCH ... avg_ns=...` artifacts (recommended):
  - Pros: deterministic pass/fail math and rerun handling, better evidence quality.
  - Cons: small extra tooling maintenance.

## Validation Architecture

### Evidence Layout (recommended)
Create one evidence bundle per run family:

```text
.planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/
  macos-metal/
    baseline/
      commands.log
      compare.txt
      bench-run1.txt
      bench-run2.txt        # only if rerun needed
      bench-eval.md
    candidate/
      commands.log
      compare.txt
      bench-run1.txt
      bench-run2.txt        # only if rerun needed
      bench-eval.md
      manual-smoke.md
  windows-vulkan-msvc/
    baseline/
      commands.log
      compare.txt
      bench-run1.txt
      bench-run2.txt        # only if rerun needed
      bench-eval.md
      manual-smoke.md
    candidate/
      commands.log
      compare.txt
      bench-run1.txt
      bench-run2.txt        # only if rerun needed
      bench-eval.md
      manual-smoke.md
  windows-vulkan-mingw-smoke/
    commands.log
    smoke-notes.md
```

### Required Commands: macOS Metal

```bash
# Build + validate
cmake -B build -G Ninja
ninja -C build mdcad_math_harness
./build/bin/mdcad_math_harness --list
./build/bin/mdcad_math_harness --mode compare --strict
./build/bin/mdcad_math_harness --mode bench --iterations 20000
ninja -C build math-validation

# Manual smoke
./build/bin/mdCAD
```

### Required Commands: Windows Vulkan (MSVC hard gate)

```powershell
# Configure + build Vulkan path
cmake -B build-vulkan -G "Visual Studio 18" -DUSE_VULKAN=ON
cmake --build build-vulkan --config Release --target mdcad_math_harness

# Harness gates
.\build-vulkan\bin\Release\mdcad_math_harness.exe --list
.\build-vulkan\bin\Release\mdcad_math_harness.exe --mode compare --strict
.\build-vulkan\bin\Release\mdcad_math_harness.exe --mode bench --iterations 20000
cmake --build build-vulkan --config Release --target math-validation

# Manual smoke
.\build-vulkan\bin\Release\mdCAD.exe
```

### Required Commands: Windows Vulkan (MinGW smoke-only)

```powershell
cmake -B build-mingw -G "MinGW Makefiles"
cmake --build build-mingw
.\build-mingw\bin\mdcad_math_harness.exe --mode compare --strict
.\build-mingw\bin\mdCAD.exe
```

### Benchmark Evaluation Rule (for PERF-02 and PERF-03)
For each case and platform:

```text
slowdown_pct = ((candidate_avg_ns - baseline_avg_ns) / baseline_avg_ns) * 100
pass if slowdown_pct <= 5.0
if 5.0 < slowdown_pct <= 8.0: allow one rerun and use lower candidate avg_ns
```

Use all current harness bench IDs as gating set:
- `legacy-mat4-mul`
- `cglm-mat4-mul`
- `legacy-mat4-inverse`
- `cglm-mat4-inv`
- `legacy-screen-ray`
- `cglm-screen-ray`
- `bench-interaction-ray`
- `bench-interaction-drag`
- `bench-quat-ops`

### Manual Windows Vulkan Checklist (HOT-04)
- camera orbit/pan/zoom across dense geometry
- pick hover/click on thin lines and points
- gizmo axis drag (no jump, stable delta)
- gizmo plane drag across multiple camera angles
- geometry-mode vertex drag on transformed entities
- undo/redo after drag workflows
- import/save sanity (load scene/import sample, save, reload)

## Concrete Verification Strategy by Requirement

### HOT-04
- Automated:
  - `--mode compare --strict` must pass on `build-vulkan`.
  - `math-validation` must pass in `build-vulkan`.
- Manual:
  - full Windows Vulkan checklist above, with concise pass/fail notes.
- Evidence:
  - `windows-vulkan-msvc/*` command log, compare output, checklist summary.
- Fail policy:
  - Phase remains open until MSVC Vulkan gate is green.

### PERF-02
- Baseline:
  - capture macOS baseline bench artifact at phase start.
- Candidate:
  - rerun after each meaningful hardening change.
- Pass:
  - every bench case `<= 5%` slowdown vs macOS baseline (rerun rule allowed once).
- Evidence:
  - macOS `bench-run*.txt` + `bench-eval.md` with per-case table and decision.

### PERF-03
- Baseline:
  - capture Windows MSVC Vulkan baseline bench artifact at phase start.
- Candidate:
  - rerun after each meaningful hardening change.
- Pass:
  - every bench case `<= 5%` slowdown vs Windows baseline (rerun rule allowed once).
- Evidence:
  - Windows Vulkan `bench-run*.txt` + `bench-eval.md` with per-case table and decision.

## Risk Register and Mitigations

| ID | Risk | Likelihood | Impact | Trigger | Mitigation | Owner/Plan |
|----|------|------------|--------|---------|------------|------------|
| R1 | Vulkan pick/readback hitch persists under required workflows | High | High | Stutter during hover/drag on Windows Vulkan | Implement Option B (persistent resources + fence), measure before/after | 05-01 |
| R2 | Queue-sync changes break pick correctness (wrong hover ID / stale pixels) | Medium | High | compare pass but manual pick fails | Add targeted pick sanity smoke after each readback change; keep fallback path for quick rollback | 05-01 |
| R3 | Benchmark noise causes false failures | Medium | Medium | cases fluctuate around threshold | lock power profile/run conditions; enforce rerun rule and keep raw artifacts | 05-02 |
| R4 | Cross-toolchain confusion (MSVC vs MinGW) leads to wrong gate interpretation | Medium | Medium | MinGW passes while MSVC fails | enforce explicit gate matrix in plan docs and verification checklist | 05-01 |
| R5 | Scope creep into non-gating refactors delays closure | Medium | High | unrelated cleanup appears in Phase 5 tasks | keep tasks tied to HOT-04/PERF-02/PERF-03; backlog everything else via 05-03 table | 05-03 |
| R6 | Baseline artifacts are missing or inconsistent, invalidating PERF claims | Medium | High | cannot compute trustworthy slowdown % | make baseline capture first task in 05-02 and treat missing baseline as hard block | 05-02 |
| R7 | Evidence quality too weak for sign-off | Medium | High | outputs not reproducible or incomplete | standardize artifact tree + required files + summary template | 05-01/05-02 |

## Explicit Plan Slicing Recommendation (3 Plans)

### 05-01: Windows Vulkan stability and hard-gate readiness
**Primary objective:** close HOT-04 behavior gate on MSVC Vulkan.

Scope:
- run initial Windows Vulkan hard-gate workflow and capture baseline evidence
- harden Vulkan pick/readback hotspot (Option B preferred)
- update docs/checklist language where needed for repeatable Windows gate runs
- run MinGW smoke for secondary signal only

Exit criteria:
- MSVC Vulkan strict compare pass
- MSVC Vulkan manual checklist pass
- reproducible hitch either resolved or bounded with accepted mitigation and residual-risk note

### 05-02: Native performance gate execution and interpretation
**Primary objective:** close PERF-02 + PERF-03 with per-case evidence.

Scope:
- capture/freeze baselines for macOS and Windows Vulkan
- run candidate benchmark series on both targets
- apply one-rerun policy for marginal misses
- compute per-case slowdown decisions and produce gate report

Exit criteria:
- all macOS and Windows cases within tolerance under defined policy
- archived raw outputs + interpreted per-case table for both platforms

### 05-03: Closure hardening and next-wave backlog handoff
**Primary objective:** resolve residual blockers and package clean handoff.

Scope:
- fix remaining gating issues (if any) or land smallest safe mitigations
- document collateral findings that do not block phase requirements
- produce structured backlog table (P1/P2/P3) and mirror key items into `STATE.md`

Exit criteria:
- HOT-04, PERF-02, PERF-03 all evidenced green
- backlog table complete across long-tail migration, platform expansion, and residual hardening/perf work

## Planning Notes for the Next Agent
- Treat the first measurable run on each platform as baseline capture; do not backfill from memory.
- Keep performance claims tied to raw `BENCH ... avg_ns=...` artifacts, not screenshots or narrative.
- Keep manual smoke concise and binary (`pass/fail + short note`) to avoid ambiguous sign-off.
- If Option B cannot clear hitch risk without destabilization, stop at smallest safe mitigation and explicitly keep phase open rather than forcing premature closure.

## Plan-Readiness Checklist
- [x] Hard gates are explicit and mapped to requirement IDs.
- [x] Vulkan hardening options include tradeoffs and recommended default.
- [x] Risk register includes triggers and mitigation owners.
- [x] 3-plan slicing recommendation is explicit (`05-01`, `05-02`, `05-03`).
- [x] Validation architecture includes commands and artifacts for macOS + Windows Vulkan.

---
*Research completed: 2026-03-25*
*Ready for planning: yes*
