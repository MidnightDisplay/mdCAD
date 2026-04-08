# Pitfalls Research: v1.4 Solver Robustness + Sketch Gizmo Corrections

**Domain:** Failure modes for v1.4 solver and gizmo work  
**Researched:** 2026-04-08  
**Confidence:** High

| Pitfall | Why It Happens | Detection | Prevention | Suggested Phase |
|---|---|---|---|---|
| Selection-order dependent line-line results | Participant normalization is inconsistent | Same constraints give different outcomes by selection order | Canonical participant ordering and direction rules | 26 |
| Group constraint over-constraint loops | Naive pairwise expansion adds redundant equations | Pass-count spikes and intermittent unsatisfied errors | Minimal relation model + redundancy checks | 26 |
| ALONG line frame mismatch | Axis interpreted in wrong frame or overly strict endpoint lock | Immediate ALONG unsatisfied errors in valid setups | Directional line equations + frame-correct axis handling | 27 |
| Tangency drag branch instability | Multiple tangent branches without stable interaction anchor | Jitter, flips, or drag freeze near corner cases | Anchor persistence + step clamping + warm-start | 27 |
| Partial commit on failed solve | Candidate state isolation is incomplete | Sketch mutates after failed recalc | Strict commit-on-success transactional boundary | 27 |
| Gizmo authority split (transform vs geometry) | Legacy transform path overlaps endpoint semantics | Line moves inconsistently or snaps back | Active-sketch line path updates geometry endpoints only | 28 |
| Midpoint anchor drift | Gizmo origin sourced from stale transform values | Gizmo appears off-line or jumps on drag | Midpoint recomputed from current `A/B` each update | 28 |
| Opaque blocked drags | Diagnostics do not clearly map to blocking constraints | User sees no movement with no actionable reason | Deterministic implication payload + participant highlighting | 28 |

## Immediate Warning Signals

- Any deterministic replay mismatch under identical inputs.
- Any valid ALONG line authoring producing immediate unsatisfied-driving errors.
- Any tangency failure that leaves solver non-responsive for subsequent edits.
- Any active-sketch line gizmo drag that changes transform but not geometry endpoints.
