# Coding Conventions

**Analysis Date:** 2026-03-24

## Naming Patterns

**Files:**
- `snake_case` for source and headers, with `src/` modules typically using short feature names like `src/app.c`, `src/math3d.h`, `src/ui/ui_scene_hierarchy.h`.
- Header-only modules are the norm; most reusable code lives in `.h` files with include guards, while `src/app.c` remains the main translation unit.
- Script/tooling files keep native extensions and platform conventions: `.py`, `.mjs`, `.ps1`, `.sh`, plus platform-specific `.m` for Objective-C (`src/gpu/pick_readback_metal.m`).

**Functions:**
- `snake_case` for all C/C++-side functions, usually prefixed by the owning module, such as `ecs_world_init()`, `scene_add_point()`, `ui_scene_hierarchy_draw()`, and `mat4_mul()`.
- `static inline` is the dominant function pattern in headers; public helpers are exposed directly from the header rather than through separate `.c` implementations.
- Event-style helpers also stay `snake_case` rather than using `handle*` or `on*` prefixes.

**Variables:**
- `snake_case` for local variables, parameters, struct fields, and state objects.
- Simple short loop indices such as `i`, `j`, and `k` are common in tight loops.
- No underscore prefix convention for private members, because the codebase is C-first and mostly struct-based.

**Types:**
- `PascalCase` for typedef names that model concepts, such as `TransformComp`, `GeometryComp`, `LightComp`, and `undo_redo_t`.
- Enums use `snake_case` type names with `UPPER_SNAKE_CASE` values in many places, for example `geometry_type_t` and `GEOM_TRIANGLE`.
- Structs and enums are typically paired with factory/default helpers like `transform_comp_default()` and `light_comp_point()`.

## Code Style

**Formatting:**
- No formatter config is checked in; the code follows a consistent hand-written style.
- Four-space indentation is the prevailing style across C, CMake, JavaScript, Python, and shell snippets.
- Braces usually stay on the same line for functions and control blocks, with section separators like `//------------------------------------------------------------------------------`.
- Semicolons are used where the language requires them; Python scripts follow standard Python syntax without extra tooling.

**Linting:**
- No repo-wide lint configuration is present for C/C++ or Node scripts.
- `scripts/package.json` does not define a lint target; `npm test` is a placeholder that exits with an error.
- Style validation is therefore mostly review-based and compile-based rather than tool-enforced.

## Import Organization

**Order:**
1. Platform and external SDK headers first in entrypoints, for example `src/app.c` includes `platform.h`, Sokol headers, then cimgui.
2. Project headers next, grouped by subsystem with comments such as `// Project modules`, `// ECS modules`, and `// Gizmo system`.
3. Relative includes are used for local module composition, e.g. `../ecs/ecs_world.h` from `src/ui/ui_scene_hierarchy.h`.

**Grouping:**
- Include groups are separated by blank lines and often annotated with comment banners.
- Within modules, related declarations are grouped into labeled sections such as `Types`, `Initialization`, `Error codes`, `Geometry creation helpers`, and `Scene Save`.

**Path Aliases:**
- No include path aliasing convention is used in source.
- Module-relative includes and explicit `../` paths are preferred over virtual aliases.

## Error Handling

**Patterns:**
- Functions usually return `bool`, enum error codes, `0`, `NULL`, or `0`-equivalent values for failure instead of throwing.
- Parsing code favors early returns and explicit status enums, such as `ply_error_t` in `src/ply_loader.h` and `jsonl_error_t` in `src/jsonl_loader.h`.
- Memory ownership is manual and paired with explicit `*_init()`, `*_free()`, and `*_shutdown()` helpers.

**Error Types:**
- Invalid input and file problems are surfaced through error enums and human-readable string helpers, not exceptions.
- Invariants are usually guarded with conservative fallbacks, such as returning identity matrices or empty results when inputs are unusable.
- Runtime failure reporting in the app leans on Sokol/cimgui logger hooks and UI status strings rather than a central exception/logging framework.

## Logging

**Framework:**
- The native app passes `slog_func` to Sokol and cimgui setup in `src/app.c`.
- Tooling scripts use standard console output (`print`, `console.log`) for progress and diagnostics.

**Patterns:**
- Logging is mostly diagnostic and boundary-level, not a pervasive application service.
- Scripts print command progress and artifact paths, while the app relies on status text in the UI for user-facing feedback.
- There is no dedicated structured logging library in the repo.

## Comments

**When to Comment:**
- Comments are used heavily for subsystem banners, ownership, and reasoning around edge cases.
- The codebase documents why a workaround exists, especially for platform quirks, GPU picking, binary parsing, and undo/redo behavior.
- Obvious line-by-line comments are avoided when the code is self-explanatory.

**JSDoc/TSDoc:**
- JavaScript and Python scripts use docstrings for module purpose and CLI usage, but there is no formal JSDoc/TSDoc requirement.
- C headers use block comments for module summaries and function intent instead of API docs.

**TODO Comments:**
- TODO-style notes are not standardized beyond occasional inline comments.
- Long-lived work is tracked in the active `.planning/` docs (especially `.planning/STATE.md`, roadmap artifacts, and phase/quick-task plans) instead of relying on TODO comments.

## Function Design

**Size:**
- Small, composable helpers are preferred, especially in header-only modules.
- Larger features are broken into focused files like `src/ply_import_job.h`, `src/ply_mesh_import_job.h`, and `src/jsonl_import_job.h`.

**Parameters:**
- Parameter lists are explicit and usually stay positional; related values are often passed as separate scalars or small structs.
- Output parameters are used where a function needs to return multiple values, especially in math and parsing helpers.

**Return Values:**
- Functions return early on invalid state, empty input, or allocation failure.
- Many helpers return simple status values rather than adopting a `Result<T, E>`-style abstraction.

## Module Design

**Exports:**
- The codebase is intentionally C-style and does not use namespaces.
- Public module entry points are prefixed by their domain, such as `ui_*`, `ecs_*`, `scene_*`, `geometry_*`, `ply_*`, and `jsonl_*`.

**Barrel Files:**
- Barrel files are not part of the style.
- Modules are included directly from the owning header, and the top-level app wires subsystems together explicitly.

*Convention analysis: 2026-03-24*
*Update when patterns change*
