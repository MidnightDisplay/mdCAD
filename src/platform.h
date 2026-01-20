//------------------------------------------------------------------------------
// platform.h - Platform and backend detection (header-only)
//------------------------------------------------------------------------------
#ifndef PLATFORM_H
#define PLATFORM_H

//------------------------------------------------------------------------------
// Backend detection for Sokol
//------------------------------------------------------------------------------
#if defined(_WIN32)
    #define SOKOL_D3D11
    #define PLATFORM_WINDOWS
#elif defined(__EMSCRIPTEN__)
    #define SOKOL_WGPU
    #define PLATFORM_WEB
#elif defined(__APPLE__)
    #define SOKOL_METAL
    #define PLATFORM_MACOS
#else
    #define SOKOL_GLCORE
    #define PLATFORM_LINUX
#endif

#endif // PLATFORM_H
