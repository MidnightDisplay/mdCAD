# Phase 1: Selection and Conventions - Research

**Researched:** 2026-03-24
**Domain:** C-first math backend adoption for a cross-platform graphics runtime
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** `cglm` `0.9.6` is the chosen math backend for the first migration milestone.
- **D-02:** The backend choice only reopens for a hard build-system or license blocker.
- **D-03:** Planning should not carry an active fallback candidate; if `cglm` fails on a hard blocker, the project should explicitly reopen research instead of shadow-planning backups.
- **D-04:** The migration path is direct `cglm` adoption in mdCAD code, not a long-lived project-owned compatibility facade.
- **D-05:** This intentionally supersedes the current `FOUND-02` / Phase 2 assumption that mdCAD will expose a stable compatibility layer around the vendor library; downstream planning should revise that roadmap/requirements language instead of preserving it.
- **D-06:** `cglm` API family may vary by subsystem. Planners and executors may choose struct or array-style usage per subsystem instead of enforcing one global style, as long as the global conventions and alignment contract remain consistent.
- **D-07:** Phase 1 is performance-first with respect to `cglm` adoption; the project may accept early alignment work to unlock the stronger SIMD path.
- **D-08:** Data-layout refactors are acceptable when needed to satisfy the chosen alignment strategy, including hot structs and other math-heavy storage, not only thin adapters.
- **D-09:** mdCAD must define one global alignment policy with explicit rules and enforcement rather than allowing per-subsystem drift or implicit compiler defaults.
- **D-10:** If a supported native gate cannot satisfy the chosen alignment policy cleanly, the migration slice should block instead of silently dropping to a weaker fallback mode.
- **D-11:** The migration will standardize mdCAD on a new project-wide math convention instead of preserving `src/math3d.h` semantics as the internal baseline.
- **D-12:** The target convention is column-major matrices, right-handed coordinates, and clip depth `0..1`.
- **D-13:** That convention is global across supported backends; backend glue may adapt, but the math semantics should not fork per backend.
- **D-14:** During staged rollout, migrated runtime slices should still normalize user-visible behavior so camera, picking, and gizmo interaction feel remain stable while the internal convention changes land.
- **D-15:** Phase 1 starts with header-only / inline `cglm`.
- **D-16:** Initial vendoring should use the least disruptive CMake wiring possible, centered on header vendoring and minimal include-path changes.
- **D-17:** Linked `cglm` is deferred; reconsider it only if measurable compile-time, binary-size, or runtime pressure appears after initial adoption.

### the agent's Discretion
- Choose struct vs array `cglm` API usage per subsystem based on code shape and measurable cost.
- Decide the exact enforcement mechanism for the global alignment policy, such as compile-time assertions, helper headers, or targeted macros.
- Decide how to normalize visible behavior during rollout while the internal `0..1` clip-depth convention is introduced.
- Decide the exact file layout for vendored headers and minimal CMake wiring as long as it stays lightweight and consistent with existing build patterns.

### Deferred Ideas (OUT OF SCOPE)
None.
</user_constraints>

<research_summary>
## Summary

Phase 1 no longer needs to choose between candidates. The research problem is now narrower: how to operationalize the locked `cglm` `0.9.6` choice in a way that matches mdCAD's build constraints, direct-adoption preference, and performance-first alignment policy. The earlier project research already established that `cglm` fits the license, language, and graphics-helper requirements; this phase-specific research focuses on converting those findings into a concrete first-step execution shape.

The most robust Phase 1 implementation is a docs-first plus build-first sequence. First, reconcile roadmap and requirement language with the direct-adoption decision so later plans stop assuming a broad compatibility facade. Second, vendor `cglm` headers and prove they compile in the existing native build via one thin project-owned include entrypoint. Third, codify the new global math contract in one project-owned header and one supporting doc, then wire that contract into the entrypoint so future migration slices start from explicit conventions rather than file-local assumptions.

**Primary recommendation:** Execute Phase 1 as three sequential plans: planning-doc reconciliation, minimal `cglm` vendoring/build proof, and convention/alignment contract definition wired into a thin include entrypoint.
</research_summary>

<standard_stack>
## Standard Stack

The established libraries/tools for this phase:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `cglm` | `0.9.6` | Chosen C math backend | Matches the locked backend decision, supports header-only integration, and provides graphics-oriented helpers with documented SIMD behavior |
| CMake + Ninja | Existing repo workflow | Native build validation | Already the active native path on this machine and the least disruptive place to prove the vendor integration |
| Project-owned convention header | New repo artifact | Central math contract | Needed because direct `cglm` adoption still requires one authoritative place for layout, clip depth, handedness, and alignment rules |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `cglm/cglm.h` | `0.9.6` | Array/inline API | Use in subsystems where low-level array-oriented access is the cleanest fit |
| `cglm/struct.h` | `0.9.6` | Struct API | Use in subsystems where field-style ergonomics reduce migration friction |
| `rg` | Existing tool | Static verification | Use to verify exact strings and policy markers in docs and headers during Phase 1 execution |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Direct `cglm` adoption | Project-owned compatibility facade | Safer abstraction boundary, but it directly conflicts with the user's locked adoption decision |
| Header-only vendoring | Linked call API | Could reduce compile pressure later, but adds premature build complexity without a measured need in Phase 1 |

**Installation:**
```bash
# Phase 1 should vendor upstream cglm 0.9.6 into the repo, not fetch it at build time.
cmake -B build -G Ninja
ninja -C build
```
</standard_stack>

<architecture_patterns>
## Architecture Patterns

### Recommended Project Structure
```text
vendors/
└── cglm/
    ├── include/cglm/...
    ├── LICENSE
    └── VERSION.txt

src/
└── math/
    ├── cglm_entry.h
    └── math_conventions.h

docs/
├── MATH_BACKEND_DECISION.md
└── MATH_CONVENTIONS.md
```

### Pattern 1: Thin Entry Point, Not a Wrapper API
**What:** Use one project-owned include entrypoint for vendored `cglm` headers and policy macros without recreating `vec3_t` / `mat4_t` wrappers.
**When to use:** When the project wants direct vendor adoption but still needs a single place to enforce include order, compile flags, and future hotspot guidance.
**Example:**
```c
// src/math/cglm_entry.h
#ifndef MDCAD_CGLM_ENTRY_H
#define MDCAD_CGLM_ENTRY_H

#include "math_conventions.h"
#include <cglm/cglm.h>
#include <cglm/struct.h>

static inline void mdcad_cglm_compile_anchor(void) {
    (void)sizeof(mat4);
    (void)sizeof(mat4s);
}

#endif
```

### Pattern 2: Convention Capsule
**What:** Record matrix layout, handedness, clip depth, alignment posture, and rollout normalization rules in one project-owned header plus one explanatory doc.
**When to use:** Always, because direct adoption across camera/ECS/gizmo/render code will drift quickly if the contract remains implicit.
**Example:**
```c
// src/math/math_conventions.h
#define MDCAD_MATH_MATRIX_COLUMN_MAJOR 1
#define MDCAD_MATH_HANDEDNESS_RIGHT_HANDED 1
#define MDCAD_MATH_CLIP_DEPTH_ZERO_TO_ONE 1
#define MDCAD_MATH_ALIGNMENT_POLICY_PERFORMANCE_FIRST 1
```

### Pattern 3: Docs Before Build-Wiring
**What:** Update roadmap/requirements/state/README before touching the build so later execution uses the correct source of truth.
**When to use:** When user decisions intentionally override current roadmap or requirement language, as they do here.
**Example:**
```markdown
1. Update ROADMAP.md Phase 2 away from "compatibility layer"
2. Update REQUIREMENTS.md FOUND-02 to a thin entrypoint wording
3. Update STATE.md and README.md to reflect the active math-foundation migration
```

### Anti-Patterns to Avoid
- **Reintroducing a wrapper-first phase contract:** It contradicts D-04 and D-05 and will keep later planning anchored to the wrong architecture.
- **Letting each subsystem pick conventions ad hoc:** Mixed clip-space and alignment assumptions will create silent camera, pick, and render drift.
- **Adding linked `cglm` support before it is measured:** This spends Phase 1 budget on a hypothetical optimization instead of closing the currently locked decisions.
</architecture_patterns>

<dont_hand_roll>
## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Backend evaluation redo | A second candidate tournament | Existing project research + `docs/MATH_BACKEND_DECISION.md` | The backend is already locked; reopening it now only adds churn |
| Broad compatibility shim | New `vec3_t` / `mat4_t` wrapper surface | Direct `cglm` adoption with `src/math/cglm_entry.h` | Wrapper maintenance is exactly what the user rejected |
| Convention guessing | Per-file comments and tribal knowledge | `src/math/math_conventions.h` + `docs/MATH_CONVENTIONS.md` | Hotspot math needs one authoritative contract |

**Key insight:** The main risk in Phase 1 is not missing a library feature. It is allowing stale roadmap assumptions and implicit math conventions to survive into later migration phases.
</dont_hand_roll>

<common_pitfalls>
## Common Pitfalls

### Pitfall 1: Wrapper Drift Survives the Decision
**What goes wrong:** The plans still create a compatibility facade because the older roadmap and requirement language were never reconciled.
**Why it happens:** Existing research and roadmap files were written before the user locked direct adoption.
**How to avoid:** Put roadmap/requirements/state reconciliation in the first plan and make later plans depend on it.
**Warning signs:** Phase 2 still mentions "compatibility layer" or tasks introduce `typedef vec3s vec3_t`.

### Pitfall 2: Build Proof Without a Real Compile Anchor
**What goes wrong:** `cglm` headers are vendored, but the main target never compiles them, so build success does not actually prove integration.
**Why it happens:** Header-only libraries can look integrated even when no compiled translation unit includes them.
**How to avoid:** Add a thin entrypoint header and reference it from a compiled source file in the main target.
**Warning signs:** Include paths are added, but no compiled file contains `#include "math/cglm_entry.h"`.

### Pitfall 3: Conventions Documented in a Doc but Not in Code
**What goes wrong:** The contract exists only in markdown, so future subsystem work can bypass it accidentally.
**Why it happens:** Teams treat convention capture as documentation only.
**How to avoid:** Create both a code-owned header and a supporting doc, and make the entrypoint include the header.
**Warning signs:** `docs/MATH_CONVENTIONS.md` exists, but no file under `src/` includes or references the same constants.
</common_pitfalls>

<code_examples>
## Code Examples

Verified patterns from project research and current code:

### Minimal header-only compile anchor
```c
// Derived from the current header-only style in src/ and the direct-adoption decision.
#include "math/cglm_entry.h"

static void init(void) {
    mdcad_cglm_compile_anchor();
}
```

### Existing column-major baseline in mdCAD
```c
typedef struct { float m[16]; } mat4_t;  // column-major
```

### Existing hotspot shape that needs explicit convention guidance
```c
mat4_t view = orbit_camera_get_view_matrix(&state.camera);
mat4_t proj = mat4_perspective(0.785398f, aspect, 0.1f, 100.0f);
mat4_t vp_mat = mat4_mul(proj, view);
```
</code_examples>

<sota_updates>
## State of the Art (2024-2025)

What's changed recently:

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Custom local graphics math | Adopt maintained SIMD-aware C libraries like `cglm` | Matured over recent library releases | Lets mdCAD stop growing `src/math3d.h` for common graphics helpers |
| Implicit clip-space assumptions | Explicit clip-depth / handedness configuration in project-owned contracts | Now expected in multi-backend graphics codebases | Phase 1 must name the global convention before hotspot migration |
| "Header-only means no integration plan" | Header-only plus compile anchor plus policy header | Common modern C adoption pattern | Phase 1 can stay lightweight without being sloppy |

**New tools/patterns to consider:**
- Thin include entrypoints for header-only vendor libraries
- Compile-time assertions around matrix size/alignment assumptions in hotspot foundations

**Deprecated/outdated:**
- Rebuilding a local math layer from scratch
- Treating documentation and compiled policy headers as interchangeable
</sota_updates>

## Validation Architecture

- **Primary gate:** `cmake -B build -G Ninja && ninja -C build`
- **Static policy gate:** `rg -n "cglm 0\\.9\\.6|thin project-owned math entrypoint|MDCAD_MATH_CLIP_DEPTH_ZERO_TO_ONE|MDCAD_MATH_ALIGNMENT_POLICY_PERFORMANCE_FIRST" .planning/ROADMAP.md .planning/REQUIREMENTS.md .planning/STATE.md README.md docs/MATH_BACKEND_DECISION.md docs/MATH_CONVENTIONS.md src/math/cglm_entry.h src/math/math_conventions.h`
- **Feedback cadence:** run the static policy gate after each task that touches docs/headers; run the native build gate after each plan wave that touches compiled sources or CMake
- **Manual verification:** none required for Phase 1 planning artifacts; native build success plus static policy checks are the main acceptance path

<open_questions>
## Open Questions

1. **Exact `cglm` macro set for the chosen convention contract**
   - What we know: the project wants `0..1` clip depth and a performance-first alignment stance.
   - What's unclear: the final set of `cglm` configuration macros to enable in `cglm_entry.h`.
   - Recommendation: resolve this in Plan 01-03 when writing `math_conventions.h`, then keep the chosen macro set in the same file and document it in `docs/MATH_CONVENTIONS.md`.

2. **How far Phase 2 renaming should go**
   - What we know: the current "Compatibility Layer and Validation Harness" title is stale.
   - What's unclear: the final exact wording for the renamed Phase 2 and any downstream plan bullet adjustments.
   - Recommendation: resolve it in Plan 01-01 while updating roadmap and requirement language so future planning starts from the corrected phase map.
</open_questions>

<sources>
## Sources

### Primary (HIGH confidence)
- `.planning/phases/01-selection-and-conventions/01-CONTEXT.md` — locked user decisions for this phase
- `.planning/research/SUMMARY.md` — backend recommendation, risks, and milestone framing
- `.planning/research/STACK.md` — `cglm` API modes, version recommendation, and adoption tradeoffs
- `.planning/research/ARCHITECTURE.md` — previous wrapper-first architecture, now used mainly as a contrast to the direct-adoption decision
- `src/math3d.h`, `src/orbit_camera.h`, `src/app.c`, `src/components/component_types.h`, `src/components/transform_comp.h`, `src/gpu/geometry_batch.h` — current codebase coupling points and convention-sensitive hotspots

### Secondary (MEDIUM confidence)
- https://github.com/recp/cglm — upstream project and license reference already captured in project research
- https://cglm.readthedocs.io/en/latest/build.html — upstream integration modes already captured in project research
- https://cglm.readthedocs.io/en/latest/api.html — upstream API split already captured in project research

### Tertiary (LOW confidence - needs validation)
- None. Phase 1 planning relies on existing project research plus local code inspection.
</sources>

<metadata>
## Metadata

**Research scope:**
- Core technology: `cglm` direct adoption
- Ecosystem: header-only vendoring, CMake include-path wiring, project-owned convention headers
- Patterns: thin entrypoint, convention capsule, docs-first roadmap reconciliation
- Pitfalls: wrapper drift, fake build proof, doc-only convention capture

**Confidence breakdown:**
- Standard stack: HIGH - backend and build path are already locked by context plus existing research
- Architecture: HIGH - the remaining ambiguity is execution ordering, not core direction
- Pitfalls: HIGH - directly supported by current codebase hotspots and prior research
- Code examples: MEDIUM - examples are project-shaped patterns rather than copied upstream snippets

**Research date:** 2026-03-24
**Valid until:** 2026-04-23
</metadata>

---

*Phase: 01-selection-and-conventions*
*Research completed: 2026-03-24*
*Ready for planning: yes*
