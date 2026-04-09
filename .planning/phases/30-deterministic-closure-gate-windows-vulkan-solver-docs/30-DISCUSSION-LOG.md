# Phase 30: Deterministic Closure Gate (Windows Vulkan) + Solver Docs - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-04-09
**Phase:** 30-deterministic-closure-gate-windows-vulkan-solver-docs
**Areas discussed:** Validation gate scope/commands, Determinism sign-off policy, Solver docs structure/location, Literature references depth

---

## Validation gate scope/commands

| Option | Description | Selected |
|--------|-------------|----------|
| Keep exact 7-test suite | Use established canonical closure gate surface from prior phases | ✓ |
| Expand tests now | Add additional suites in Phase 30 closure gate | |
| Narrow to smoke subset | Use smaller subset for faster closure | |

**User's choice:** Keep exact 7-test suite.
**Notes:** User also locked canonical regex command string and required baseline plus mandatory immediate rerun. Windows Vulkan gate must include build + canonical gate + rerun.

---

## Determinism sign-off policy

| Option | Description | Selected |
|--------|-------------|----------|
| Flaky = failure | Stabilize before closure; no known-fail closure path | ✓ |
| Allow one flaky rerun | Accept instability if final rerun passes | |
| Known-fail defer | Document and close anyway | |

**User's choice:** Flaky behavior is treated as failure.
**Notes:** Evidence policy remains command + result summaries (raw logs optional), and closure remains Windows Vulkan scoped in this phase.

---

## Solver docs structure/location

| Option | Description | Selected |
|--------|-------------|----------|
| Dedicated doc in `docs/solver/` | Create focused architecture document for maintainers | ✓ |
| Expand README only | Put architecture in top-level README section | |
| Planning-only doc | Keep docs in `.planning/` artifacts only | |

**User's choice:** `docs/solver/SOLVER_ARCHITECTURE.md`.
**Notes:** Structure locked: Overview -> Solve pipeline -> Diagnostics flow -> Code anchors -> TL;DR debug primer. Include file + key function anchors by stage.

---

## Literature references depth

| Option | Description | Selected |
|--------|-------------|----------|
| Concise curated references | 3-6 high-signal references with practical rationale | ✓ |
| Broad bibliography | 10+ references for wider coverage | |
| Minimal refs | 1-2 links only | |

**User's choice:** Concise curated references with practical emphasis.
**Notes:** Prioritize practical geometric-constraint and numerical-robustness references; TL;DR must include failure-family "first place to look" entry points.

---

## the agent's Discretion

- Exact command example formatting and doc section naming/details.
- Exact verification artifact summary layout.

## Deferred Ideas

- Cross-platform closure expansion beyond Windows Vulkan.
- Any new solver capability work discovered during documentation.
