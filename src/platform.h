//------------------------------------------------------------------------------
// platform.h - Platform and backend detection (header-only)
//------------------------------------------------------------------------------
#ifndef PLATFORM_H
#define PLATFORM_H

//------------------------------------------------------------------------------
// Backend detection for Sokol
//------------------------------------------------------------------------------
#if defined(_WIN32)
    #if defined(__MINGW32__) || defined(__MINGW64__)
        // MinGW: use Vulkan for better performance
        #define SOKOL_VULKAN
    #elif defined(USE_VULKAN)
        // MSVC with Vulkan option enabled via CMake -DUSE_VULKAN=ON
        #define SOKOL_VULKAN
    #else
        // MSVC default: use D3D11
        #define SOKOL_D3D11
    #endif
    #define PLATFORM_WINDOWS
#elif defined(__EMSCRIPTEN__)
    #define SOKOL_WGPU
    #define PLATFORM_WEB
#elif defined(__ANDROID__)
    #define SOKOL_GLES3
    #define PLATFORM_ANDROID
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #define SOKOL_METAL
    #if TARGET_OS_IPHONE
        #define PLATFORM_IOS
    #else
        #define PLATFORM_MACOS
    #endif
#else
    #define SOKOL_GLCORE
    #define PLATFORM_LINUX
#endif

//------------------------------------------------------------------------------
// Rendering configuration
//------------------------------------------------------------------------------
// MSAA for the main viewport offscreen pass only.
// Pick buffer remains single-sampled (sample_count = 1).
#ifndef MDCAD_VIEWPORT_MSAA_SAMPLES
#define MDCAD_VIEWPORT_MSAA_SAMPLES 8
#endif

#endif // PLATFORM_H
