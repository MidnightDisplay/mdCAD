# Phase 15: Validation & Acceptance Closure - Research

**Researched:** 2026-04-02  
**Domain:** acceptance evidence packaging, Windows gate proof, case-study reproducibility, deferred requirement traceability  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

- Required bundle: `15-VALIDATION.md` + `15-03-SUMMARY.md` + `CHECKPOINT.md` update.
- Windows gate is strict: both Release `mdCAD` build in `build-vulkan` and full `ctest --test-dir build-vulkan -C Release --output-on-failure` must pass.
- `VAL-01`: include two representative sketch case studies plus one Script IO scenario (parse/apply/reset + diagnostics visibility).
- `VAL-03`: explicitly deferred in Phase 15 by decision; do not mark complete.

</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| VAL-01 | Ship sketch/script case studies for development/debugging | Create reproducible evidence pack with 2 sketch scenarios + 1 Script IO scenario, each with steps + expected outcomes. |
| VAL-02 | Windows MSVC + Vulkan acceptance gates pass | Record both required commands with output anchors and PASS/FAIL status. |
| VAL-03 | macOS parity executed and recorded | Mark deferred-by-decision with explicit handoff/risk note; no completion claim in Phase 15. |

</phase_requirements>

## Summary

Phase 15 should be planned as an evidence-closure phase, not feature work. Existing repo precedent (Phase 11/14/16 validation artifacts) favors requirement-level traceability, explicit command evidence, and concise closure summaries.

For `VAL-02`, evidence is only compliant when both required Windows gates are shown with command + output anchors + artifact path. Narrative-only “pass” statements are insufficient.

For `VAL-03`, use explicit deferred wording in validation/summary/checkpoint so future readers do not interpret it as complete.

## Recommended Evidence Layout

```text
.planning/phases/15-validation-and-acceptance-closure/
├── 15-VALIDATION.md
├── 15-03-SUMMARY.md
└── evidence/
    ├── windows-vulkan-msvc/
    │   ├── gate-build-mdcad.txt
    │   ├── gate-ctest-full.txt
    │   └── provenance.md
    └── case-studies/
        ├── sketch-01-*/
        ├── sketch-02-*/
        └── script-io-scenario-01/
```

## Common Pitfalls

1. Declaring Windows acceptance with only one of the two required commands.
2. Shipping case-study files without reproducible steps and expected outcomes.
3. Omitting `VAL-03` status or implying completion instead of explicit deferment.

## Validation Architecture

### Test Framework
| Property | Value |
|---|---|
| Framework | CTest + manual reproducibility walkthroughs |
| Config file | `CMakeLists.txt`, `src/CMakeLists.txt` |
| Quick run command | `cmake --build build-vulkan --config Release --target mdCAD` |
| Full suite command | `ctest --test-dir build-vulkan -C Release --output-on-failure` |

### Requirement-to-Evidence Map (planning target)
| Req ID | Evidence Type | Command / Artifact |
|---|---|---|
| VAL-01 | Case-study artifact pack | `evidence/case-studies/...` with runbook + expected outcomes |
| VAL-02 | Automated Windows gate | Build + full CTest command outputs under `evidence/windows-vulkan-msvc/` |
| VAL-03 | Deferred traceability | Explicit deferred row + follow-up handoff note |

### Wave 0 Gaps
- [ ] Create `15-VALIDATION.md` with requirement rows and deferred `VAL-03`.
- [ ] Create `15-03-SUMMARY.md` tied to concrete artifact paths.
- [ ] Add Windows gate output artifacts and provenance notes.
- [ ] Add 2 sketch + 1 Script IO reproducible case-study evidence packs.
- [ ] Update `CHECKPOINT.md` with Phase 15 closure note.

## Sources

- `.planning/phases/15-validation-and-acceptance-closure/15-CONTEXT.md`
- `.planning/ROADMAP.md`
- `.planning/REQUIREMENTS.md`
- `.planning/STATE.md`
- `docs/feature-proposal/Sketches, Constraints, Scripting.md`
- `.planning/phases/14-script-io-api-undo-integration/14-03-SUMMARY.md`
- `.planning/phases/14-script-io-api-undo-integration/14-VALIDATION.md`
- `docs/QUICKSTART.md`
- `CHECKPOINT.md`

---

## RESEARCH COMPLETE

Phase 15 research is complete and ready for planning.
