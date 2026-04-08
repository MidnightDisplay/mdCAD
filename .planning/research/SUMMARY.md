# Project Research Summary

**Project:** mdCAD  
**Domain:** v1.4 Solver Robustness + Sketch Gizmo Corrections  
**Researched:** 2026-04-08  
**Confidence:** High

## Executive Summary

v1.4 should be delivered as a robustness-first milestone: fix known solver correctness gaps, preserve deterministic transactional behavior, and correct active-sketch line gizmo semantics. No new external stack is required; success depends on legality/runtime parity, focused test coverage, and strict regression gates on Windows Vulkan.

## Key Findings

### Stack

- Reuse current C-first stack and test infrastructure.
- Add internal focused tests and one human-facing solver architecture document.
- Avoid external solver and build-system churn for this milestone.

### Feature Scope

- Add line-line `PARALLEL` and `PERPENDICULAR` for pairs and groups.
- Fix line `ALONG X/Y/Z` semantics and failures.
- Harden arc-line tangency + mixed drag determinism.
- Fix active-sketch line gizmo midpoint and endpoint authority behavior.
- Document solver architecture with literature/code references and TL;DR implementation primer.

### Architecture

- Main code touchpoints: `ecs_scene.h`, `constraint_types.h`, `app.c`, `gizmo.h`.
- Keep transactional solve semantics and deterministic failure implication lifecycle.
- Implement in dependency order: legality/runtime parity -> solver fixes -> gizmo fix -> docs.

### Top Risks

1. Legality/runtime drift for new line constraints.
2. ALONG line semantics causing immediate unsatisfied-driving failures.
3. Tangency drag instability with branch/anchor issues.
4. Transform-vs-geometry authority conflicts in gizmo handling.

## Roadmap Implications

- Start with correctness and deterministic contracts.
- Follow with robustness in mixed-constraint solve paths.
- Close with active-sketch line gizmo UX correction and documentation.

---
*Research completed: 2026-04-08*  
*Ready for requirements: yes*
