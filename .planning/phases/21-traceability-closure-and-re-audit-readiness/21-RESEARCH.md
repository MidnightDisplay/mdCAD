# Phase 21: traceability-closure-and-re-audit-readiness - Research

**Researched:** 2026-04-07  
**Domain:** Documentation traceability closure, verification authority alignment, milestone re-audit readiness  
**Confidence:** HIGH

## User Constraints (from CONTEXT.md)

### Locked Decisions
### Phase 10 evidence refresh policy
- **D-01:** Run fresh targeted manual verification for the two remaining Phase 10 human-required checks before upgrading Phase 10 closure status.
- **D-02:** Use the existing Phase 10 smoke checklist/UAT structure as the baseline execution contract and capture explicit fresh outcomes.

### Phase 10 artifact authority
- **D-03:** `10-VERIFICATION.md` is the authoritative closure artifact and must be upgraded to `passed` only with explicit citation-backed evidence.
- **D-04:** `10-HUMAN-UAT.md` (or equivalent UAT artifact) records step-by-step manual execution evidence and links back to verification rows.

### PH18-03 traceability closure policy
- **D-05:** Use traceability-first docs reconciliation for `PH18-03`: align Phase 18 summary/frontmatter and requirement mapping to the already-passed `18-VERIFICATION.md`.
- **D-06:** Run fresh targeted `PH18-03` reruns only if reconciliation reveals mismatch, ambiguity, or stale evidence.

### Milestone readiness gate
- **D-07:** Phase 21 completion requires a successful re-run of `v1.2-MILESTONE-AUDIT.md` with no remaining `human_needed`/partial gaps for `SKCH-01`, `SKCH-02`, `SKCH-03`, and `PH18-03`.
- **D-08:** Publish an explicit cross-file traceability table showing requirement-to-artifact closure alignment.

### the agent's Discretion
- Choose the minimal targeted rerun command set if evidence mismatch is discovered during reconciliation.
- Choose exact formatting/layout of final traceability tables while preserving requirement-level audit clarity.
- Choose whether `REQUIREMENTS.md` traceability table status updates are applied in the same step as artifact closure or immediately after audit rerun, as long as consistency is preserved.

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SKCH-01 | User can create a sketch entity and attach point, line, and arc/circle geometry to that sketch. | Re-close via fresh manual rerun evidence in `10-HUMAN-UAT.md` + status/citation upgrade in `10-VERIFICATION.md`; then set traceability parity in `REQUIREMENTS.md` and re-audit output. |
| SKCH-02 | User can view per-sketch solve status, color policy, geometry count, and constraint count in the Entity Inspector. | Same Phase 10 manual rerun contract (existing smoke checklist + explicit result log), citation-back evidence row refresh, and milestone audit parity check. |
| SKCH-03 | User can fix, unfix, and delete sketch geometries from GeometryManager using single-select and multi-select workflows. | Same manual rerun + one-step undo confirmation flow, authoritative verification status change, and requirement table closure synchronization. |
| PH18-03 | Endpoint undo integration preserves non-sketch manipulation and script transaction invariants, including stable arc endpoint undo behavior across angular branch boundaries. | Docs-first reconciliation: fill `18-03-SUMMARY.md` `requirements-completed` and ensure evidence mapping parity with already-passed `18-VERIFICATION.md`; only rerun targeted tests if mismatch appears. |

</phase_requirements>

## Project Constraints (from copilot-instructions.md)

- Use the get-shit-done skill when user requests GSD or uses a `gsd-*` command.
- Treat `/gsd-*` and `gsd-*` as command invocations and load matching `.github/skills/gsd-*` guidance.
- When a command says to spawn a subagent, prefer a matching custom agent from `.github/agents`.
- Do not apply GSD workflows unless explicitly requested by the user.
- After completing a `gsd-*` deliverable, offer the user a next step (`ask_user`) until the user indicates they are done.

## Summary

Phase 21 is an audit-closure phase, not an implementation phase. The core work is evidence-state normalization across existing artifacts: clear Phase 10 `human_needed` with fresh manual reruns for the two explicit human tests, close PH18-03 summary/frontmatter completeness debt, and then re-run the milestone audit so requirement status, verification status, and summary frontmatter agree.

The highest-risk failure mode is “paper closure drift” (updating one file but not its authoritative counterpart). This phase must treat `10-VERIFICATION.md` and `18-VERIFICATION.md` as source-of-truth anchors, with UAT and summary docs as supporting artifacts. The re-audit should only pass when SKCH-01/02/03 and PH18-03 are no longer partial and all cross-file traceability rows are citation-backed.

**Primary recommendation:** Execute Phase 21 as a 3-step closure pipeline: (1) Phase 10 fresh human rerun + authority upgrade, (2) PH18-03 docs parity reconciliation, (3) milestone re-audit + explicit cross-file closure matrix publication.

## Standard Stack

### Core
| Library/Tool | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Markdown phase artifacts (`*-VERIFICATION.md`, `*-HUMAN-UAT.md`, `*-SUMMARY.md`) | repo-native | Authoritative traceability + evidence capture | Existing project contract already enforces these artifacts as closure authority |
| Node.js + `gsd-tools.cjs` | v25.8.1 | Phase init/context and audit workflow operations | Official workflow entrypoint in this repo’s GSD stack |
| CMake/CTest | 4.3.0 / 4.3.0 | Targeted rerun evidence when mismatch requires revalidation | Existing verification artifacts use these commands; reproducible and already integrated |

### Supporting
| Library/Tool | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `10-sketch-managers-smoke-checklist.md` | repo-native | Manual rerun contract for SKCH-01..03 | Mandatory for D-01/D-02 phase-10 closure policy |
| `v1.2-MILESTONE-AUDIT.md` | repo-native | Final requirement/phase/integration parity gate | Mandatory completion gate per D-07 |
| `.planning/REQUIREMENTS.md` traceability table | repo-native | Requirement-to-phase status normalization | Update once verification authority status is final |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Fresh Phase 10 manual rerun | Evidence reuse only | Rejected by locked decision D-01; leaves audit ambiguity |
| PH18-03 docs-first reconciliation | Always rerun tests | Unnecessary churn unless mismatch exists (D-06 controls escalation) |

## Architecture Patterns

### Recommended Project Structure
```text
.planning/
├── REQUIREMENTS.md                                   # Requirement status source
├── v1.2-MILESTONE-AUDIT.md                           # Final gate report
└── phases/
    ├── 10-sketch-foundations-managers/
    │   ├── 10-HUMAN-UAT.md                           # Manual execution evidence
    │   └── 10-VERIFICATION.md                         # Authoritative closure status
    └── 18-add-undo-steps-for-endpoint-moves/
        ├── 18-03-SUMMARY.md                          # requirements-completed parity
        └── 18-VERIFICATION.md                         # PH18-03 authoritative evidence
```

### Pattern 1: Authoritative-Artifact-First Closure
**What:** Update supporting docs only after authoritative verification artifacts are complete and citation-backed.  
**When to use:** Any traceability closure where status drift exists (`human_needed`, `partial`, empty `requirements-completed`).  
**Example:**
```markdown
# in 10-VERIFICATION.md frontmatter
status: passed
human_verification:
  - test: "Dual-entrypoint sketch attach flow"
    result: pass
    evidence_ref: "10-HUMAN-UAT.md#test-1"
```

### Pattern 2: Three-Source Parity Check
**What:** For each requirement, verify consistency across REQUIREMENTS traceability row, phase verification status, and summary frontmatter completion list.  
**When to use:** Milestone re-audit preparation and final gate rerun.  

### Anti-Patterns to Avoid
- **Status-only edit:** Changing `status` field without row-level evidence updates/citations.
- **Summary-leading authority:** Marking `requirements-completed` without matching authoritative verification rows.
- **Broad rerun reflex:** Running full regression suites when docs parity alone resolves PH18-03.

## Plan Decomposition Candidates

1. **Plan 21-01 — Phase 10 Fresh Human Closure (SKCH-01/02/03)**  
   - Run manual checklist items for the two required human checks.  
   - Update `10-HUMAN-UAT.md` with fresh timestamps/results.  
   - Upgrade `10-VERIFICATION.md` to `passed` with explicit citation links.

2. **Plan 21-02 — PH18-03 Requirement Completeness Reconciliation**  
   - Reconcile `18-03-SUMMARY.md` `requirements-completed` with `18-VERIFICATION.md` PH18-03 pass state.  
   - Ensure `REQUIREMENTS.md` PH18-03 row status can move from pending to complete once parity is proven.  
   - Trigger targeted rerun only if evidence mismatch exists.

3. **Plan 21-03 — Milestone Re-Audit and Closure Matrix Publication**  
   - Re-run/update `v1.2-MILESTONE-AUDIT.md`.  
   - Publish explicit cross-file traceability matrix for SKCH-01/02/03 + PH18-03.  
   - Confirm zero residual `human_needed`/partial gaps for Phase 21 targets.

**Dependency order:** 21-01 → 21-02 → 21-03 (strict).  
**Reason:** milestone re-audit should be last, after all source artifacts are already aligned.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Manual UI verification schema | New ad-hoc checklist format | Existing `10-sketch-managers-smoke-checklist.md` + `10-HUMAN-UAT.md` contract | Preserves comparability with prior evidence and audit expectations |
| Requirement parity logic | Custom interpretation per file | 3-source parity check (`REQUIREMENTS` + `VERIFICATION` + `SUMMARY`) | Prevents “closed in one file only” false positives |
| PH18-03 retest policy | Always rerun everything | Decision-gated rerun (only on mismatch) | Saves time and prevents needless churn while preserving confidence |

**Key insight:** In docs-first closure phases, consistency beats novelty; reuse existing evidence contracts and only escalate execution when parity checks detect ambiguity.

## Common Pitfalls

### Pitfall 1: Authority Inversion
**What goes wrong:** UAT/summary is updated, but authoritative verification remains stale.  
**Why it happens:** Teams treat convenience docs as source-of-truth.  
**How to avoid:** Always update `*-VERIFICATION.md` first (or in same commit), then sync supporting artifacts.  
**Warning signs:** `status: human_needed` remains while UAT says pass.

### Pitfall 2: Silent Partials in Frontmatter
**What goes wrong:** Requirement still appears partial because `requirements-completed` is empty/incomplete despite passing verification.  
**Why it happens:** Summary frontmatter is skipped during reconciliation.  
**How to avoid:** Include frontmatter parity checklist item per target requirement.  
**Warning signs:** Audit marks requirement `partial` with “summary frontmatter empty” evidence.

### Pitfall 3: Audit Before Closure
**What goes wrong:** Re-audit is run too early and produces noisy failures that obscure real blockers.  
**Why it happens:** Missing plan sequencing discipline.  
**How to avoid:** Require artifact parity completion gate before rerunning milestone audit.  
**Warning signs:** Repeated audits with unchanged blockers.

## Code Examples

### Requirement closure matrix row format
```markdown
| Requirement | REQUIREMENTS.md | VERIFICATION.md | SUMMARY frontmatter | Final |
|-------------|-----------------|-----------------|---------------------|-------|
| SKCH-01     | Phase 21 / Complete | 10-VERIFICATION: passed (citation) | N/A (Phase-level summary not required) | Closed |
```

### Targeted rerun escalation trigger (PH18-03 only if mismatch)
```text
if (summary_frontmatter_missing || verification_citation_ambiguous || stale_evidence_detected):
    run: ctest -R "endpoint_pick|scene_solver_contract|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure
else:
    docs-only reconciliation (no rerun)
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| “Implementation done” accepted without strict artifact parity | 3-source audit gate with requirement-level traceability evidence | v1.2 milestone audit process | Makes documentation debt visible and blocks archival until closed |
| Broad reruns for every closure pass | Targeted reruns only on evidence ambiguity | Phase 19/20 closure pattern | Faster closure cycles with explicit confidence boundaries |

**Deprecated/outdated:**
- “Summary-only completion”: outdated for this repo because milestone audits explicitly cross-check verification authority and summary/frontmatter parity.

## Open Questions

1. **Does milestone re-audit in this phase include full 10–20 scope rerun or targeted gap-only refresh?**
   - What we know: D-07 requires no residual gaps for SKCH-01/02/03 and PH18-03.
   - What's unclear: Whether maintainers expect complete score refresh for prior closed gaps too.
   - Recommendation: Recompute full report but explicitly label Phase 21 closure section as targeted objective.

2. **Should `REQUIREMENTS.md` be updated immediately after each sub-closure or once after re-audit?**
   - What we know: Sequencing is discretionary if consistency is preserved.
   - What's unclear: Team preference for commit granularity.
   - Recommendation: Update after each authoritative closure artifact change to reduce drift windows.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| node | gsd-tools init + audit scripts | ✓ | v25.8.1 | — |
| cmake | build/verify commands in artifacts | ✓ | cmake version 4.3.0 | — |
| ctest | targeted regression re-checks | ✓ | ctest version 4.3.0 | manual-only evidence review (lower confidence) |
| mdCAD.exe (build artifact) | Phase 10 manual rerun/UAT | ✓ | build artifact present | rebuild target mdCAD |
| VULKAN_SDK env | Windows Vulkan build if rebuild needed | ✓ | C:\VulkanSDK\1.4.341.1 | Set env var per CHECKPOINT.md |

**Missing dependencies with no fallback:** None.  
**Missing dependencies with fallback:** None currently missing.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CTest (CMake-native test registration) |
| Config file | CMake/CTest defaults (no dedicated pytest/jest config) |
| Quick run command | `ctest -R "endpoint_pick\|scene_solver_contract\|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure` *(only if PH18-03 mismatch is detected)* |
| Full suite command | `ctest --test-dir build-vulkan -C Release --output-on-failure` *(not default for this docs-first phase)* |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SKCH-01 | Sketch create + attach from required entrypoints | manual UAT | `cmake --build build-vulkan --config Release --target mdCAD` (prep only) | ✅ (`10-HUMAN-UAT.md`, checklist) |
| SKCH-02 | Inspector sketch status/count surfaces | manual UAT | `cmake --build build-vulkan --config Release --target mdCAD` (prep only) | ✅ (`10-HUMAN-UAT.md`, checklist) |
| SKCH-03 | GeometryManager fix/unfix/delete + single-step undo | manual UAT | `cmake --build build-vulkan --config Release --target mdCAD` (prep only) | ✅ (`10-HUMAN-UAT.md`, checklist) |
| PH18-03 | Non-sketch/script invariants + arc endpoint branch stability | docs-first + conditional targeted regression | `ctest -R "endpoint_pick\|scene_solver_contract\|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure` *(conditional)* | ✅ (`18-VERIFICATION.md`, tests present) |

### Sampling Rate
- **Per task commit:** Artifact parity self-check (`REQUIREMENTS` ↔ `VERIFICATION` ↔ `SUMMARY`) plus relevant command evidence if rerun triggered.
- **Per wave merge:** Targeted conditional regression command (PH18-03 only when mismatch/ambiguity found).
- **Phase gate:** Updated `v1.2-MILESTONE-AUDIT.md` shows no `human_needed`/partial for SKCH-01/02/03/PH18-03.

### Wave 0 Gaps
- [ ] Add explicit fresh execution timestamps in `10-HUMAN-UAT.md` for the two required manual checks (currently historical run only).
- [ ] Add/refresh explicit citation pointers in `10-VERIFICATION.md` to the fresh UAT rows before status upgrade to `passed`.
- [ ] Populate `18-03-SUMMARY.md` `requirements-completed` with `PH18-03` to eliminate known partial-gap trigger.

## Sources

### Primary (HIGH confidence)
- `.planning/phases/21-traceability-closure-and-re-audit-readiness/21-CONTEXT.md` — locked decisions and scope constraints.
- `.planning/REQUIREMENTS.md` — requirement definitions + current traceability statuses (SKCH-01..03 and PH18-03 pending in Phase 21).
- `.planning/v1.2-MILESTONE-AUDIT.md` — explicit residual gap evidence and final gate contract.
- `.planning/phases/10-sketch-foundations-managers/10-VERIFICATION.md` — current `human_needed` authority state + required human tests.
- `.planning/phases/10-sketch-foundations-managers/10-HUMAN-UAT.md` — existing manual evidence structure to reuse.
- `.planning/phases/10-sketch-foundations-managers/evidence/10-sketch-managers-smoke-checklist.md` — concrete manual rerun checklist.
- `.planning/phases/18-add-undo-steps-for-endpoint-moves/18-VERIFICATION.md` — PH18-03 already passed in authoritative verification artifact.
- `.planning/phases/18-add-undo-steps-for-endpoint-moves/18-03-SUMMARY.md` — known frontmatter parity gap (`requirements-completed: []`).
- `.planning/config.json` — `workflow.nyquist_validation: true`, so validation architecture is required.
- `.github/copilot-instructions.md` — project-level GSD workflow directives.

### Secondary (MEDIUM confidence)
- `CHECKPOINT.md` — command patterns and environment assumptions used to shape manual gate recommendations.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — based on repo-native workflow contracts and currently installed toolchain.
- Architecture: HIGH — constrained by explicit Phase 21 decisions and established artifact authority patterns.
- Pitfalls: HIGH — directly evidenced by known residual audit gaps (`human_needed`, empty summary frontmatter, partial statuses).

**Research date:** 2026-04-07  
**Valid until:** 2026-05-07 (stable process domain; re-check if workflow templates change)
