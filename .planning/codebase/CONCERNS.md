# Codebase Concerns

**Analysis Date:** 2026-03-24

## Tech Debt

**Backend-specific pick readback split across `src/gpu/pick_readback.h`, `src/gpu/pick_readback_vulkan.c`, and `src/gpu/pick_readback_opengl.c`:**
- Issue: WebGPU is still a stub, while Vulkan and OpenGL each use bespoke native-handle code paths.
- Why: Sokol does not expose one uniform readback API, so each backend was wired separately.
- Impact: Hover, selection, and gizmo hit-testing can diverge by platform and break silently after backend updates.
- Fix approach: Add a narrow shared adapter layer and a backend regression check for pick readback on each supported renderer.

**Manual parsing and allocation in `src/jsonl_loader.h`, `src/ply_loader.h`, `src/jsonl_import_job.h`, and `src/ply_mesh_import_job.h`:**
- Issue: The import stack uses fixed-size line buffers, chunked state machines, and many direct `malloc`/`realloc` calls.
- Why: It was built to keep huge imports interactive with progress reporting.
- Impact: Large records drive high memory usage, and unchecked `realloc` assignment can lose the old pointer on allocation failure.
- Fix approach: Wrap growth in checked helpers, add hard size limits for pathological lines, and fuzz the parser entry points.

**Docs and comments drift in `README.md`, `docs/QUICKSTART.md`, and `src/gpu/pick_readback.h`:**
- Issue: The README still says the web target is `WebGL2`, while the codebase/docs around the web path and GPU picking describe `SOKOL_WGPU`/WebGPU; `src/gpu/pick_readback.h` also still labels Vulkan readback as a placeholder even though `src/gpu/pick_readback_vulkan.c` and `CHANGELOG.md` document a real implementation.
- Why: Backend names changed faster than the front-page copy.
- Impact: New contributors can build the wrong mental model for the web target and expect desktop-style save/load on iOS or Android.
- Fix approach: Align the platform tables with `src/platform.h` and the current backend matrix, then keep mobile filesystem caveats explicit.

## Known Bugs

**Web picking is effectively disabled on the WebGPU path (`src/gpu/pick_readback.h`):**
- Symptoms: Hover, selection, and gizmo hit-testing fall back to background-only behavior on web builds.
- Trigger: Any `SOKOL_WGPU` build, because `pick_readback_pixels()` returns `false` and clears the buffer.
- Workaround: None besides avoiding the web build for picking-heavy workflows.
- Root cause: WebGPU readback is still a placeholder.

**Vulkan picking stalls the queue per cursor update (`src/gpu/pick_readback_vulkan.c`):**
- Symptoms: Hover-driven interaction can hitch on slower GPUs or larger scenes.
- Trigger: Any frame that needs a pick-buffer readback on Vulkan.
- Workaround: Reduce how often the pick buffer is rebuilt.
- Root cause: The implementation allocates a staging buffer and calls `vkQueueWaitIdle()` for every readback.

**OpenGL readback can fail silently when FBO entry points are unavailable (`src/gpu/pick_readback_opengl.c`):**
- Symptoms: Selection clears as if nothing is under the cursor.
- Trigger: Desktop OpenGL systems where `glGenFramebuffers` or `glFramebufferTexture2D` cannot be resolved through the current path.
- Workaround: Use a backend with implemented readback.
- Root cause: Readback depends on runtime function resolution plus a temporary FBO path.

**OpenGL/GLES readback is not wired into the build in `src/CMakeLists.txt`:**
- Symptoms: Linux or Android-style builds that need `pick_readback_pixels()` can fail at link time or drift from the documented backend matrix.
- Trigger: Non-Apple, non-Windows builds where `src/platform.h` selects `SOKOL_GLCORE` or `SOKOL_GLES3`.
- Workaround: Manually add `src/gpu/pick_readback_opengl.c` to the relevant build before relying on that path.
- Root cause: `src/CMakeLists.txt` only adds Metal, D3D11, and Vulkan readback sources.

## Security Considerations

**Local file import/export in `src/ui/ui_file_browser.h` and the loader entry points:**
- Risk: The app trusts local paths and large JSON/PLY payloads; malformed files can drive high memory use or parser failure.
- Current mitigation: No remote input surface; imports are user-initiated.
- Recommendations: Add file-size checks, clearer error surfacing, and fuzz coverage for `src/jsonl_loader.h` and `src/ply_loader.h`.

**Web/mobile filesystem assumptions in `docs/QUICKSTART.md` and `src/ui/ui_file_browser.h`:**
- Risk: iOS, Android, and web targets do not have a conventional desktop filesystem, so save/load UX can mislead users.
- Current mitigation: Docs warn about it, but the UI still exposes desktop-style flows.
- Recommendations: Gate unsupported actions by platform and route mobile/web I/O through explicit picker or browser APIs.

## Performance Bottlenecks

**`src/gpu/pick_readback_vulkan.c`:**
- Problem: Per-hover readback does a fresh staging-buffer allocation plus `vkQueueWaitIdle()`.
- Measurement: No profile numbers checked in; the stall is obvious from the synchronous design.
- Cause: Simplicity over asynchronous GPU transfer handling.
- Improvement path: Reuse a persistent staging buffer and fence the transfer instead of idling the queue.

**`src/jsonl_loader.h` and `src/ply_loader.h`:**
- Problem: Large-file parsing allocates megabyte-scale line buffers and many intermediate arrays.
- Measurement: No hard numbers recorded.
- Cause: The parsers favor simplicity and chunked UI progress over tight memory bounds.
- Improvement path: Stream with reusable buffers, then add size-based short-circuiting for pathological records.

## Fragile Areas

**`src/gpu/pick_buffer.h` plus the backend readback adapters:**
- Why fragile: The pick buffer is only as reliable as backend readback, and failures collapse to "no hover" with little diagnosis.
- Common failures: Silent loss of hover, selection, and gizmo picking on one platform-specific path.
- Safe modification: Change one backend at a time and verify `src/ui/ui_pick_debug.h` plus cursor interaction in the native Ninja `build/` workflow on the M1 MacBook Air.
- Test coverage: No automated cross-backend pick regression tests.

**`src/app.c` drag bookkeeping:**
- Why fragile: The gizmo path snapshots pointer arrays manually and must free them on every drag end or cancel path.
- Common failures: Leaks or stale state if drag termination is interrupted.
- Safe modification: Keep allocation and free paired in one helper, then exercise both transform and geometry edit modes.
- Test coverage: No dedicated unit tests; this is still manual-interaction heavy.

**`src/scene_serializer.h`:**
- Why fragile: Save/load logic is a custom parser and builder with version checks, manual entity mapping, and batch parenting.
- Common failures: New component types are easy to omit, and malformed JSON can fail late.
- Safe modification: Add a round-trip fixture per entity type before changing the parser.
- Test coverage: No snapshot round-trip tests in-tree.

## Scaling Limits

**Pick buffer and hover detection in `src/gpu/pick_buffer.h`:**
- Current capacity: 20x20 pixel readback buffer with cursor-centered sampling.
- Limit: Dense overlaps and very thin geometry become hard to hit.
- Symptoms at limit: Missed picks or unstable hover under heavy scene overlap.
- Scaling path: Increase sampling resolution or add adaptive zoom for difficult targets.

**Import pipelines in `src/jsonl_import_job.h` and `src/ply_mesh_import_job.h`:**
- Current capacity: Chunked import is tuned for interactive progress, not throughput.
- Limit: Very large mesh and log files still consume significant RAM while arrays are expanded.
- Symptoms at limit: Long import times and memory spikes.
- Scaling path: Move more parsing to streaming/no-copy structures and reuse parsed buffers.

## Dependencies at Risk

**Sokol internals in `src/gpu/pick_readback_vulkan.c`:**
- Risk: The Vulkan readback path depends on a custom `sg_vk_query_image_info_ext()` hook and Sokol private layout assumptions.
- Impact: A Sokol upgrade can break Vulkan picking without touching mdCAD code directly.
- Migration plan: Track the exact Sokol revision and prefer an upstream-supported query API when one exists.

**`cJSON`-based import in `src/jsonl_loader.h`:**
- Risk: Mesh JSONL parsing assumes `cJSON` can comfortably handle very large per-line documents.
- Impact: Oversized logs can become memory-heavy or fail to parse.
- Migration plan: Keep a streaming fallback or tighten the accepted file-format contract.

## Missing Critical Features

**WebGPU readback in `src/gpu/pick_readback.h`:**
- Problem: Web builds still lack real GPU picking.
- Current workaround: None.
- Blocks: Hover, selection, and gizmo interaction parity on web.
- Implementation complexity: Medium, because async buffer mapping and timing need to be integrated cleanly.

**Mobile file I/O parity noted in `docs/QUICKSTART.md`:**
- Problem: iOS and Android builds do not have a desktop-style filesystem workflow.
- Current workaround: Manual platform-specific handling.
- Blocks: Load/save UX parity with desktop.
- Implementation complexity: Medium-high, because platform pickers and sandboxed storage need separate flows.

## Test Coverage Gaps

**Backend matrix coverage across `src/gpu/pick_readback_*` and `src/app.c`:**
- What's not tested: Cross-backend picking on Metal, D3D11, Vulkan, OpenGL, and WebGPU.
- Risk: Silent regressions in cursor hover and gizmo hit-testing.
- Priority: High.
- Difficulty to test: Different toolchains and runtime environments.

**Import/serialization round-trips in `src/scene_serializer.h`, `src/ply_loader.h`, and `src/jsonl_loader.h`:**
- What's not tested: Save/load round-trips for triangles, meshes, lights, and nested parent hierarchies.
- Risk: Data loss or mismatched geometry after import/export.
- Priority: High.
- Difficulty to test: Needs fixture files and deterministic entity comparisons.

**Manual UI flows in `src/ui/ui_scene_hierarchy.h` and `src/ui/ui_file_browser.h`:**
- What's not tested: File browser navigation, import modal state machines, and cancel/retry flows.
- Risk: UI regressions in the largest stateful panel.
- Priority: Medium.
- Difficulty to test: Requires integration-style UI automation.

*Concerns audit: 2026-03-24*
*Update as issues are fixed or new ones are discovered*
