# Phase 7 Importer Targeted Checklist

Use this checklist for targeted Phase 7 importer validation (light gate + explicit parity evidence).

## Preconditions

- Build is current for Windows Vulkan path:
  - `cmake --build build-vulkan --config Release --target mdcad_math_harness`
- Migrated files are present:
  - `src/math/math_import.h`
  - `src/jsonl_loader.h`
  - `src/jsonl_import_job.h`
  - `src/ply_loader.h`
  - `src/ply_import_job.h`
  - `src/ply_mesh_import_job.h`

## Sample Matrix (Representative + Variant/Converted)

| Sample ID | Format | Path | Mode | placement | orientation | scale | entity count | triangle count | parenting structure | PASS/FAIL | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|
| JSONL-REP | JSONL | `<representative-jsonl>` | Standard import | ☐ | ☐ | ☐ | ☐ | N/A | ☐ | ☐ | |
| JSONL-VAR | JSONL | `<variant-or-converted-jsonl>` | Standard import | ☐ | ☐ | ☐ | ☐ | N/A | ☐ | ☐ | |
| PLY-POINT-REP | PLY point cloud | `<representative-ply-point>` | Point cloud / editable | ☐ | ☐ | ☐ | ☐ | N/A | ☐ | ☐ | |
| PLY-POINT-VAR | PLY point cloud | `<variant-or-converted-ply-point>` | Point cloud / editable | ☐ | ☐ | ☐ | ☐ | N/A | ☐ | ☐ | |
| PLY-MESH-REP | PLY mesh | `<representative-ply-mesh>` | Single mesh / triangles | ☐ | ☐ | ☐ | ☐ | ☐ | ☐ | ☐ | |
| PLY-MESH-VAR | PLY mesh | `<variant-or-converted-ply-mesh>` | Single mesh / triangles | ☐ | ☐ | ☐ | ☐ | ☐ | ☐ | ☐ | |

## Required Recording Fields

For each sample row, record:

- placement
- orientation
- scale
- entity count
- triangle count (when applicable)
- parenting structure
- PASS/FAIL verdict

## Correctness Delta vs Previous Import Behavior

For every visible change (or explicit "No visible delta"), capture:

- **Implementation Analysis Reference:** touched importer path and function (example: `src/jsonl_import_job.h::jsonl_import_job_apply_transforms`)
- **Mathematical Justification:** why the behavior is now correct

## Chunked Import / Progress Semantics

| Check | PASS/FAIL | Evidence |
|---|---|---|
| JSONL chunk parse/create/parent progress behavior unchanged | ☐ | |
| PLY point import chunk/progress behavior unchanged | ☐ | |
| PLY mesh import chunk/progress behavior unchanged | ☐ | |
| Any regression treated as blocker | ☐ | |

## Blocker Handling

If any row/check is FAIL, mark blocker explicitly in report and stop phase closure until resolved.
