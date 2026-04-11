# Phase 34: Deterministic v1.5 Closure Gate - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-11
**Phase:** 34-deterministic-v1-5-closure-gate
**Areas discussed:** Closure gate command contract and platform scope, evidence strictness policy, divergence policy, historical warning-marker handling

---

## Closure gate command contract and platform scope

| Option | Description | Selected |
|--------|-------------|----------|
| Keep Phase 30 contract exactly | Windows Vulkan only, canonical 7-test command string, baseline + immediate rerun both required | ✓ |
| Keep Windows Vulkan + same 7 tests, allow command-format variants | Equivalent command variants accepted for closure evidence | |
| Keep Windows Vulkan required + optional extra supplementary platform runs | Additional platforms allowed as non-blocking evidence | |

**User's choice:** Keep Phase 30 contract exactly.
**Notes:** User explicitly preferred strict continuity with prior deterministic closure policy.

---

## Evidence strictness policy

| Option | Description | Selected |
|--------|-------------|----------|
| Command + baseline/rerun result summaries only | Keep artifact compact and deterministic-signoff focused | ✓ |
| Command + summaries + raw ctest output snippets | Include partial logs in every closure artifact | |
| Full raw logs mandatory | Require complete log dumps for closure acceptance | |

**User's choice:** Command + baseline/rerun result summaries only.
**Notes:** Raw logs remain optional diagnostic aid, not closure requirement.

---

## Divergence and flake handling

| Option | Description | Selected |
|--------|-------------|----------|
| Hard fail closure on baseline/rerun divergence | Stabilize first, then retry closure | ✓ |
| Allow one extra retry before decision | Retry once to screen transient failures | |
| Proceed with known-issue warning | Accept closure with divergence noted | |

**User's choice:** Hard fail closure; stabilize before sign-off.
**Notes:** Reinforces strict deterministic milestone credibility.

---

## Historical warning-marker handling

| Option | Description | Selected |
|--------|-------------|----------|
| Treat as non-blocking metadata after DIAG-03 passes | Historical UAT/lifecycle warning markers do not block closure | ✓ |
| Require explicit normalization cleanup before sign-off | Clear all warning markers to close phase | |
| Case-by-case reviewer decision | Leave handling discretionary at closure time | |

**User's choice:** Treat as non-blocking historical metadata once DIAG-03 gate passes.
**Notes:** Closure should be gated by deterministic evidence, not legacy marker presence.

---

## the agent's Discretion

- Exact formatting of verification summary sections and optional debug appendices.

## Deferred Ideas

None.
