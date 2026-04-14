# Quick Task 260414-mkp: Enable 4x MSAA for main viewport only

## Goal

Enable 4x MSAA for the main viewport offscreen render path, while keeping GPU picking (pick buffer and pick debug viewport texture) single-sampled.

## Task 1

- **files:** `src/platform.h`, `src/render_target.h`, `src/app.c`, `src/ui/ui_controls.h`, `src/gpu/geometry_batch.h`, `src/gizmo/gizmo_rendering.h`
- **action:** Add a shared viewport MSAA sample count config (`4`), create an MSAA offscreen color attachment + resolve attachment for viewport rendering, wire resolve attachment into the offscreen pass, and set scene/gizmo pipelines to matching sample count.
- **verify:** `cmake --build build-vulkan --config Release --target mdCAD`
- **done:** Build succeeds and pick buffer code path remains explicitly single-sample (`sample_count = 1`).
