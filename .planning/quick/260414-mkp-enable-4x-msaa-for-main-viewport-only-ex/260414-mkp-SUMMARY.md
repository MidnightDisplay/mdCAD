# Quick Task 260414-mkp Summary

## Description

Enabled 4x MSAA for the main viewport offscreen render path only, without changing pick buffer sampling.

## What changed

- Added shared config:
  - `src/platform.h` → `#define MDCAD_VIEWPORT_MSAA_SAMPLES 4`
- Updated offscreen viewport render target to proper MSAA resolve flow:
  - `src/render_target.h`
    - Added multisampled color attachment image.
    - Added single-sample resolve image used for viewport texture sampling.
    - Added resolve attachment view and sample count tracking.
- Wired resolve attachment into viewport offscreen pass:
  - `src/app.c` (`sg_begin_pass(...attachments.resolves[0]...)`)
- Aligned viewport-render pipelines to MSAA sample count:
  - `src/gpu/geometry_batch.h` (line/point/triangle pipelines)
  - `src/gizmo/gizmo_rendering.h` (line/point pipelines)
- Set offscreen pass store policy for resolved MSAA path:
  - `src/ui/ui_controls.h` (`SG_STOREACTION_DONTCARE` when MSAA is enabled)

## Explicit non-goals preserved

- `src/gpu/pick_buffer.h` remains `sample_count = 1` for both pick color and depth images.
- No changes to pick debug viewport texture path.

## Verification

- `cmake --build build-vulkan --config Release --target mdCAD` ✅
