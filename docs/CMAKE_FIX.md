# CMake Build Fixes

Summary of fixes required to build the project on macOS.

## CMake Fixes

### vendors/libsokol/CMakeLists.txt

1. **Lines 14-18**: Moved `target_include_directories` to after `add_library` - the target must exist before setting its properties.

2. **Line 29**: Added quotes around `"Darwin"` for proper string comparison:
   ```cmake
   if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
   ```

3. **Line 46**: Changed directory path to library target name - `target_link_libraries` must link to targets, not directories:
   ```cmake
   # Before: target_link_libraries(libsokol PUBLIC ${libcimgui_SOURCE_DIR})
   target_link_libraries(libsokol PUBLIC libcimgui)
   ```

4. **Line 51**: Added `${libsokol_SOURCE_DIR}/util` to INTERFACE includes so consumers can find `sokol_imgui.h`.

### src/CMakeLists.txt

5. **Line 9**: Added `libcimgui` to link libraries for include path propagation:
   ```cmake
   target_link_libraries(${PROJECT_NAME} libsokol libcimgui)
   ```

## Source Fixes

### vendors/libsokol/sokol.c

1. **Line 18**: Removed duplicate `#include "sokol_log.h"` which caused redefinition errors.

### src/demo.c

2. **Line 13**: Removed `#define SOKOL_IMGUI_IMPL` - the implementation is already compiled in `sokol.c`.

3. **Line 21**: Updated to new sokol API:
   ```c
   // Before: .context = sapp_sgcontext(),
   .environment = sglue_environment(),
   ```

4. **Line 48**: Updated render pass API:
   ```c
   // Before: sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());
   sg_begin_pass(&(sg_pass){ .swapchain = sglue_swapchain(), .action = state.pass_action });
   ```
