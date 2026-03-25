# Phase 5: Windows Vulkan Hardening and Performance Gates - Context

**Gathered:** 2026-03-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Prove the migrated math hotspot set is stable on Windows Vulkan, close native performance gates on macOS Metal and Windows Vulkan, and capture next-wave migration backlog items without adding new runtime capabilities.

</domain>

<decisions>
## Implementation Decisions

### Windows validation matrix
- **D-01:** `MSVC + Vulkan` is the only hard Windows release gate for Phase 5; `MinGW + Vulkan` is smoke-only.
- **D-02:** Windows Vulkan manual validation must run the full interaction smoke: Phase 4 interaction checklist plus scene navigation and import/save sanity.
- **D-03:** Validation evidence must include command log, harness outputs, and a concise manual checklist summary.
- **D-04:** Phase 5 cannot be marked complete until the Windows Vulkan hard gate is green.

### Performance gate policy
- **D-05:** Performance pass/fail is evaluated per benchmark case (not aggregate-only) on each native target.
- **D-06:** "No regression" threshold is `<= 5%` slowdown per benchmark case.
- **D-07:** If a case narrowly misses threshold, allow one rerun and use the better (`lower avg_ns`) result.
- **D-08:** All current benchmark cases in `src/math_harness.c` are gating for this phase.

### Vulkan hardening scope
- **D-09:** Reproducible Vulkan-only pick/readback hitches discovered during gate runs should be fixed in Phase 5.
- **D-10:** If a full structural fix is too large for Phase 5, land the smallest safe mitigation, document measured residual risk, and keep the phase open only if gate criteria still fail.
- **D-11:** Mandatory Windows Vulkan runtime hardening workflows are camera navigation, pick/hover, gizmo axis/plane drag, vertex drag, and undo/redo.
- **D-12:** Non-Windows regressions found during gate runs are tracked as collateral findings and do not block Phase 5 unless they impact `HOT-04`, `PERF-02`, or `PERF-03`.

### Backlog handoff format
- **D-13:** Next-wave capture uses a structured backlog table per item: `area`, `issue`, `evidence`, `impact`, `recommended phase`.
- **D-14:** Backlog must include long-tail migration work, platform expansion work, and residual performance/hardening work.
- **D-15:** Backlog items use three priority tiers: `P1 must-next`, `P2 should-next`, `P3 future`.
- **D-16:** Backlog is recorded in this phase context deferred section and mirrored in `STATE.md` notes.

### the agent's Discretion
- Exact command/log formatting for benchmark evidence capture.
- Exact implementation approach for Vulkan mitigation/fix, as long as Phase 5 gates and thresholds are upheld.
- Exact checklist template shape, as long as it captures the required workflows and evidence bundle.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Scope and requirement gates
- `.planning/ROADMAP.md` — Phase 5 goal, success criteria, and plan breakdown (`05-01`..`05-03`).
- `.planning/REQUIREMENTS.md` — `HOT-04`, `PERF-02`, `PERF-03` gates and deferred `TAIL-*`/`PLAT-*` tracks.
- `.planning/PROJECT.md` — migration constraints, platform priority, and performance-first posture.

### Research and risk framing
- `.planning/research/SUMMARY.md` — migration strategy and native performance-gate rationale.
- `.planning/research/PITFALLS.md` — Vulkan/platform drift and performance-proof pitfalls relevant to Phase 5.

### Runtime and build integration points
- `src/platform.h` — Windows backend selection behavior (`SOKOL_VULKAN` paths).
- `src/CMakeLists.txt` — `mdcad_math_harness`, `math-regression`, `math-bench`, and `math-validation` target wiring.
- `src/math_harness.c` — compare/bench case catalog and benchmark output format (`avg_ns`).
- `src/gpu/pick_buffer.h` — pick rebuild/readback/update-hover flow and cursor-gated rebuild behavior.
- `src/gpu/geometry_batch.h` — migrated render-adjacent geometry math path tied to runtime validation.
- `src/gpu/pick_readback_vulkan.c` — Vulkan readback implementation with queue synchronization hotspot.

### Operator runbooks
- `docs/VULKAN_WINDOWS.md` — Windows Vulkan backend operational details.
- `docs/QUICKSTART.md` — macOS/Windows harness commands and manual parity smoke workflow.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/math_harness.c`: existing compare and bench case runner with stable text output (`COMPARE ...`, `BENCH ... avg_ns=...`) for gate evidence.
- `src/CMakeLists.txt`: existing native targets (`math-regression`, `math-bench`, `math-validation`) already usable on both macOS and Windows build trees.
- `docs/QUICKSTART.md`: established operator command path for both macOS Ninja and Windows Vulkan builds.

### Established Patterns
- Harness-first validation is already the standard; app launch/manual smoke is secondary confirmation.
- Runtime migration work remains parity-first, with performance gates evaluated after parity.
- Platform backend behavior is compile-time selected via `src/platform.h` and build flags.

### Integration Points
- Windows Vulkan hardening flows through `src/gpu/pick_readback_vulkan.c` and call sites in `src/gpu/pick_buffer.h` / `src/app.c`.
- Performance gate criteria are enforced via `src/math_harness.c` output and build-target orchestration in `src/CMakeLists.txt`.
- Validation evidence and workflow docs are maintained in `docs/QUICKSTART.md`, `docs/VULKAN_WINDOWS.md`, and phase planning docs.

</code_context>

<specifics>
## Specific Ideas

- Keep Windows hard-gate scope strict: Phase 5 stays open until `MSVC + Vulkan` is green.
- Use per-case benchmark gating with `<= 5%` tolerance and one rerun policy to control run-to-run noise.
- Treat Vulkan pick-readback hitching as in-scope hardening work for this phase when reproducible under gate runs.
- Capture next-wave backlog in a structured, priority-tagged format that directly feeds follow-on planning.

</specifics>

<deferred>
## Deferred Ideas

| priority | area | issue | evidence | impact | recommended phase |
|---|---|---|---|---|---|
| P1 must-next | windows-msvc-vulkan-hot-gate | HOT-04 checklist execution is blocked on current host and needs real Windows MSVC rerun closure | .planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/windows-vulkan-msvc/candidate/compare.txt | Leaves Phase 5 runtime gate unresolved and milestone cannot close safely | 06-01 |
| P2 should-next | windows-msvc-vulkan-perf-gate | PERF-03 uses host-blocked placeholder provenance and must be replaced with native Windows benchmark capture | .planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/windows-vulkan-msvc/candidate/provenance.txt | Performance gate confidence remains reduced and rollback risk is under-measured | 06-01 |
| P2 should-next | long-tail-migration | TAIL-01 migrate remaining serializer and importer math consumers still on legacy helpers | .planning/REQUIREMENTS.md | Legacy math surface remains broad and increases long-term maintenance drag | 06-02 |
| P3 future | long-tail-migration | TAIL-02 shrink temporary thin-entrypoint migration glue after staged cutover stabilizes | .planning/PROJECT.md | Migration scaffolding persists longer than needed and obscures steady-state architecture | 06-03 |
| P3 future | platform-expansion | PLAT-01 validate cglm-backed path on iOS native runtime with parity smoke and harness evidence | .planning/REQUIREMENTS.md | Mobile parity risk remains unknown beyond desktop-first rollout | 07-01 |
| P3 future | platform-expansion | PLAT-02 validate web or wasm math behavior and alignment assumptions under webgpu path | .planning/REQUIREMENTS.md | Web target may drift from native conventions without explicit gate evidence | 07-02 |

</deferred>

---

*Phase: 05-windows-vulkan-hardening-and-performance-gates*
*Context gathered: 2026-03-25*
