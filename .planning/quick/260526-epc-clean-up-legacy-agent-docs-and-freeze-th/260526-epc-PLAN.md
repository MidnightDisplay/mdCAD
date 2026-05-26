---
phase: quick-260526-epc
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - docs/legacy/pre-gsd-agent-workflow/README.md
  - docs/legacy/pre-gsd-agent-workflow/AGENTS.md
  - docs/legacy/pre-gsd-agent-workflow/CHECKPOINT.md
  - docs/legacy/pre-gsd-agent-workflow/.plans/
  - README.md
  - docs/QUICKSTART.md
  - .github/copilot-instructions.md
  - .planning/codebase/STRUCTURE.md
  - .planning/codebase/CONVENTIONS.md
autonomous: true
requirements:
  - quick-legacy-docs-freeze
must_haves:
  truths:
    - "Legacy pre-GSD agent docs are preserved in one frozen archive under docs/legacy instead of living at the repo root."
    - "Active contributor guidance points to .planning as the current planning/state source of truth."
    - "Live repo indexes no longer describe AGENTS.md, CHECKPOINT.md, or .plans as active workflow artifacts."
  artifacts:
    - path: "docs/legacy/pre-gsd-agent-workflow/README.md"
      provides: "Archive/freeze note and path rationale"
    - path: "docs/legacy/pre-gsd-agent-workflow/AGENTS.md"
      provides: "Archived legacy agent instructions"
    - path: "docs/legacy/pre-gsd-agent-workflow/CHECKPOINT.md"
      provides: "Archived legacy continuity snapshot"
    - path: "docs/legacy/pre-gsd-agent-workflow/.plans/"
      provides: "Archived legacy plan corpus"
    - path: "README.md"
      provides: "Live repo entry point aligned to .planning"
    - path: "docs/QUICKSTART.md"
      provides: "Live quickstart without active CHECKPOINT.md guidance"
    - path: ".github/copilot-instructions.md"
      provides: "Current agent guidance pointing at .planning"
    - path: ".planning/codebase/STRUCTURE.md"
      provides: "Updated live structure map"
    - path: ".planning/codebase/CONVENTIONS.md"
      provides: "Updated live workflow conventions"
  key_links:
    - from: "README.md"
      to: ".planning/STATE.md"
      via: "live workflow/source-of-truth note"
      pattern: "\\.planning/STATE\\.md"
    - from: ".github/copilot-instructions.md"
      to: ".planning"
      via: "active GSD guidance"
      pattern: "\\.planning"
    - from: ".planning/codebase/STRUCTURE.md"
      to: "docs/legacy/pre-gsd-agent-workflow/"
      via: "top-level layout description"
      pattern: "docs/legacy/pre-gsd-agent-workflow"
---

<objective>
Archive the old root-level agent workflow docs without losing history, then make the live docs consistently treat `.planning` as the active planning and continuity system.

Purpose: Remove ambiguity between frozen pre-GSD docs and the current GSD workflow.
Output: One preserved legacy archive under `docs/legacy/...` plus aligned live documentation and codebase maps.
</objective>

<execution_context>
@.github/get-shit-done/workflows/quick.md
</execution_context>

<context>
@.planning/STATE.md
@.github/copilot-instructions.md
@README.md
@docs/QUICKSTART.md
@CHECKPOINT.md
@AGENTS.md
@.planning/codebase/STRUCTURE.md
@.planning/codebase/CONVENTIONS.md
</context>

<tasks>

<task type="auto">
  <name>Task 1: Freeze the legacy workflow into one archive root</name>
  <files>docs/legacy/pre-gsd-agent-workflow/README.md, docs/legacy/pre-gsd-agent-workflow/AGENTS.md, docs/legacy/pre-gsd-agent-workflow/CHECKPOINT.md, docs/legacy/pre-gsd-agent-workflow/.plans/</files>
  <action>Choose a single durable archive shape under `docs/legacy` and use it consistently for all three legacy artifacts; use `docs/legacy/pre-gsd-agent-workflow/` as the grouped archive root so the frozen workflow stays coherent instead of scattering files directly under `docs/legacy`. Move root `AGENTS.md`, root `CHECKPOINT.md`, and the full `.plans/` tree into that archive, preserving their content rather than deleting or rewriting history. Add a short archive README that marks the folder as frozen historical context and states that `.planning` is now the active system of record.</action>
  <verify>
    <automated>powershell -NoProfile -Command "$ok = (Test-Path 'docs/legacy/pre-gsd-agent-workflow/AGENTS.md') -and (Test-Path 'docs/legacy/pre-gsd-agent-workflow/CHECKPOINT.md') -and (Test-Path 'docs/legacy/pre-gsd-agent-workflow/.plans') -and -not (Test-Path 'AGENTS.md') -and -not (Test-Path 'CHECKPOINT.md') -and -not (Test-Path '.plans'); if (-not $ok) { exit 1 }"</automated>
  </verify>
  <done>The legacy root docs and legacy plan folder exist only under the chosen `docs/legacy` archive root, and the archive is explicitly labeled frozen.</done>
</task>

<task type="auto">
  <name>Task 2: Update live contributor docs to point at .planning</name>
  <files>README.md, docs/QUICKSTART.md, .github/copilot-instructions.md</files>
  <action>Update only the live entry-point docs so they stop implying root `CHECKPOINT.md`, root `AGENTS.md`, or `.plans/` are active workflow inputs. Add or revise the minimal wording needed to make `.planning/STATE.md` and the broader `.planning` tree the active source of truth for planning and continuity. Where a legacy mention is still helpful, point to `docs/legacy/pre-gsd-agent-workflow/` as archived/frozen context only. Keep this scoped to current guidance; do not redesign the wider GSD workflow.</action>
  <verify>
    <automated>powershell -NoProfile -Command "$ok = $true; if (-not (Select-String -Path 'README.md','.github/copilot-instructions.md' -Pattern '\\.planning' -Quiet)) { $ok = $false }; if (Select-String -Path 'docs/QUICKSTART.md' -Pattern 'see CHECKPOINT\\.md' -Quiet) { $ok = $false }; if (-not $ok) { exit 1 }"</automated>
  </verify>
  <done>Live repo-facing docs consistently describe `.planning` as active and no longer direct contributors to the old root legacy docs.</done>
</task>

<task type="auto">
  <name>Task 3: Refresh active codebase maps without churning historical phase artifacts</name>
  <files>.planning/codebase/STRUCTURE.md, .planning/codebase/CONVENTIONS.md</files>
  <action>Update the active codebase index docs so they match the post-move layout: `docs/legacy/pre-gsd-agent-workflow/` is archived/frozen, while `.planning` is the active planning and continuity area. Replace stale statements that say long-lived work is tracked in `.plans/` and `CHECKPOINT.md`. Do not mass-edit `.planning/phases/**` historical plans/research unless a top-level active index explicitly needs a pointer change.</action>
  <verify>
    <automated>powershell -NoProfile -Command "$ok = (Select-String -Path '.planning/codebase/STRUCTURE.md' -Pattern 'docs/legacy/pre-gsd-agent-workflow|\\.planning' -Quiet) -and -not (Select-String -Path '.planning/codebase/CONVENTIONS.md' -Pattern '\\.plans/|CHECKPOINT\\.md' -Quiet); if (-not $ok) { exit 1 }"</automated>
  </verify>
  <done>The live codebase maps describe the archive and the current `.planning` workflow accurately, while historical phase docs remain untouched.</done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| contributor/agent → repo docs | Documentation edits can accidentally preserve stale workflow instructions or remove historical context |
| live docs → archived docs | Legacy docs must remain accessible without being presented as active guidance |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-260526-epc-01 | Tampering | legacy archive move | mitigate | Move the legacy files intact, add a freeze note, and avoid rewriting historical body content except minimal archive labeling |
| T-260526-epc-02 | Spoofing | live documentation | mitigate | Replace active root-level workflow guidance with explicit `.planning` source-of-truth wording so contributors are not misled by stale paths |
| T-260526-epc-03 | Repudiation | codebase maps | mitigate | Update the active structure/conventions docs to reflect the archive path and current planning location |
| T-260526-epc-SC | Tampering | npm/pip/cargo installs | accept | No package-manager activity is in scope for this docs-only cleanup |
</threat_model>

<verification>
- Confirm the legacy artifacts exist only under `docs/legacy/pre-gsd-agent-workflow/`.
- Confirm live docs reference `.planning` for active workflow guidance.
- Confirm `.planning/codebase/*` reflects the archive move and no longer advertises `.plans` / `CHECKPOINT.md` as active.
</verification>

<success_criteria>
- Root `AGENTS.md`, root `CHECKPOINT.md`, and root `.plans/` are gone because they were moved, not deleted.
- `docs/legacy/pre-gsd-agent-workflow/` contains the preserved legacy docs plus an explicit frozen-archive note.
- README, QUICKSTART, Copilot instructions, and active codebase maps consistently point contributors to `.planning`.
- Historical `.planning/phases/...` artifacts are left archival unless an active top-level index needed updating.
</success_criteria>

<source_audit>
- GOAL: archive legacy pre-GSD docs and make `.planning` the active source of truth — covered by Tasks 1-3.
- REQ: none separately supplied for this quick task.
- RESEARCH: none supplied; Level 0/1 repo-context scan only.
- CONTEXT:
  - single quick-task plan with 1-3 tasks — satisfied
  - preserve legacy content by moving/archive, not deleting — Task 1
  - update only live references/source-of-truth docs — Tasks 2-3
  - historical `.planning/phases/...` can remain archival — Task 3
  - explicitly decide target `docs/legacy` path shape — Task 1
</source_audit>
