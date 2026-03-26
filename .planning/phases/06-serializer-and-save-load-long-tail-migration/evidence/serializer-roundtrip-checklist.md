# Phase 6 Serializer Roundtrip Checklist

Use this checklist for targeted Phase 6 validation (light gate). This is intentionally scoped to serializer save/load behavior.

## Preconditions

- Build is current for Windows Vulkan path:
  - `cmake --build build-vulkan --config Release --target mdcad_math_harness`
- Scene serializer writes schema v2 (`format: "mdcad-scene"`, `version: 2`).

## Branch A: Representative Scene Save/Reload

### 1) Save representative scene

- Action: Save a scene from `mdCAD` that includes multiple entities and at least one parent-child chain.
- Evidence to record:
  - sample scene path
  - entity count before save
  - expected parent links
  - expected transform fields (`position`, `rotation`, `scale`)
  - expected geometry type set

### 2) Reload saved scene

- Action: Reload the same saved scene in `mdCAD`.
- Expected: load succeeds with schema v2 status message.

### 3) Verify entity count

- Check: entity count after reload equals pre-save entity count.
- Result: mark `pass` or `fail`.

### 4) Verify parent links

- Check: representative parent links remain intact after reload (include at least one multi-level chain).
- Result: mark `pass` or `fail`.

### 5) Verify transform fields

- Check: `position`, `rotation`, and `scale` values match pre-save values for sampled entities.
- Result: mark `pass` or `fail`.

### 6) Verify geometry type preservation

- Check: geometry type values are preserved after reload (for sampled entities).
- Result: mark `pass` or `fail`.

## Branch B: Converted Old-Format Scene Verification

### 1) Convert old scene file

- Command:
  - `python scripts/scene_format_convert.py --input <legacy-scene.json> --output <converted-scene.json>`
- Expected: converter exits `0` and outputs schema v2 JSON.

### 2) Load converted output

- Action: Load the converted file in `mdCAD`.
- Expected: load succeeds.

### 3) Run the same targeted checks

- Verify again:
  - entity count
  - parent links
  - position / rotation / scale
  - geometry type
- Result: mark `pass` or `fail` for each field.

## Report Format

When recording into `serializer-targeted-check-report.md` or `scene-converter-sample-report.md`, include:

- command(s) run
- sample input/output paths
- explicit pass/fail for entity count, parent links, position, rotation, scale, geometry type
- notes for any mismatch
