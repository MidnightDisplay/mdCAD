---
phase: 40-large-dump-stability-interaction-coherence
plan: 02
subsystem: hierarchy-observer-coherence
tags: [jsonl, observer, hierarchy, coherence, stress-evidence]
requires:
  - phase: 40-large-dump-stability-interaction-coherence
    provides: "Bounded coalesced refresh runtime and advisory timing channel."
provides:
  - "Repeated-cycle coherence regressions for root-stable interaction and selection remap."
  - "Hierarchy guardrails while flat anchor refresh is running."
  - "Manual very-large stress protocol and recorded checkpoint outcome."
affects: [phase-40-verification, milestone-v1.6-closure]
tech-stack:
  added: []
  patterns: ["anchor-stable selection remap", "refresh-running interaction guardrails", "hybrid hard/advisory gate evidence"]
key-files:
  modified:
    - src/tests/jsonl_flat_observer_manual_refresh_test.c
    - src/tests/jsonl_flat_observer_inspector_contract_test.c
    - src/ui/ui_scene_hierarchy.h
    - .planning/phases/40-large-dump-stability-interaction-coherence/40-STRESS-PROTOCOL.md
requirements-completed: [PERF-03]
completed: 2026-04-27
---

# Phase 40 Plan 02 Summary

**Implemented repeated-refresh coherence hardening and closed the manual very-large stress checkpoint.**

## Accomplishments

1. Added deterministic repeated-cycle coverage in `jsonl_flat_observer_manual_refresh_test.c` across small/medium/large tiers to assert:
   - stable root-anchor identity over repeated refreshes,
   - descendant selection remap back to root after replacement,
   - expected geometry count updates per cycle.
2. Strengthened inspector contract assertions for continuity copy (`Link is OFF...`, `Refresh in progress...`) in `jsonl_flat_observer_inspector_contract_test.c`.
3. Hardened `ui_scene_hierarchy_draw_entity_tree(...)` for flat observer anchors while refresh is active:
   - anchor rows show `"(refreshing)"`,
   - drag/drop and structural edits are blocked during running refresh,
   - context menu shows explicit lock reason.
4. Added and filled `.planning/.../40-STRESS-PROTOCOL.md` with hybrid-gate interpretation and recorded manual checkpoint outcome (`APPROVED`).

## Verification

- `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_(manual_refresh_test|inspector_contract_test|auto_safety_test)" --output-on-failure` passed.
- Stress protocol gate assertions passed:
  - `Very-large JSONL` marker present
  - advisory/no lockup-crash semantics present
  - `Manual run result:` present and no `TBD`

## Outcome

Plan 40-02 is complete with coherent anchor interaction behavior over repeated refreshes and manual very-large stability evidence captured under the agreed hard/advisory hybrid gate.
