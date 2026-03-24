#ifndef MDCAD_MATH_CONVENTIONS_H
#define MDCAD_MATH_CONVENTIONS_H

/*
 * mdCAD math contract for the cglm migration.
 *
 * This file replaces src/math3d.h as the internal semantic baseline for
 * migrated subsystems. Data-layout refactors are allowed when needed to honor
 * the chosen alignment strategy, but every migrated slice must share the same
 * global contract instead of drifting per subsystem or backend.
 *
 * Backend glue may adapt to platform API differences, but semantics may not
 * fork per backend. If a supported native gate cannot honor this policy
 * cleanly, the migration slice must block rather than silently falling back.
 *
 * Rollout-sensitive hotspots that must preserve visible behavior while the
 * internal convention changes land:
 * - src/orbit_camera.h
 * - src/app.c
 * - src/ecs/ecs_scene.h
 * - src/gpu/pick_buffer.h
 * - src/gizmo/gizmo.h
 */

#define MDCAD_MATH_MATRIX_COLUMN_MAJOR 1
#define MDCAD_MATH_HANDEDNESS_RIGHT_HANDED 1
#define MDCAD_MATH_CLIP_DEPTH_ZERO_TO_ONE 1
#define MDCAD_MATH_ALIGNMENT_POLICY_PERFORMANCE_FIRST 1
#define MDCAD_MATH_NATIVE_GATE_BLOCK_ON_POLICY_BREAK 1
#define MDCAD_MATH_NORMALIZE_VISIBLE_BEHAVIOR 1

#endif
