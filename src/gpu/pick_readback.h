//------------------------------------------------------------------------------
// pick_readback.h - Platform-specific GPU texture readback (header-only interface)
//
// Provides CPU readback of pick buffer texture for GPU picking.
// Platform implementations:
// - Metal: pick_readback_metal.m (uses MTLTexture getBytes)
// - OpenGL: Uses glReadPixels (implemented inline)
// - WebGPU: Not yet implemented (requires async buffer mapping)
//------------------------------------------------------------------------------
#ifndef PICK_READBACK_H
#define PICK_READBACK_H

#include "../platform.h"
#include "sokol_gfx.h"
#include <stdint.h>
#include <stdbool.h>

//------------------------------------------------------------------------------
// Cross-platform readback interface
//------------------------------------------------------------------------------

// Read pixels from an image into a CPU buffer
// Returns true on success, false on failure
// Parameters:
//   img - Sokol image to read from
//   width, height - Image dimensions
//   pixel_data - Output buffer (must be width * height * 4 bytes for RGBA8)

//------------------------------------------------------------------------------
// Platform-specific implementations
//------------------------------------------------------------------------------

#if defined(SOKOL_METAL)

// Metal implementation is in pick_readback_metal.m
// Declaration only here - implementation in .m file
bool pick_readback_pixels(sg_image img, int width, int height, uint8_t *pixel_data);

#elif defined(SOKOL_GLCORE) || defined(SOKOL_GLES3)

// OpenGL implementation - inline
// Note: This requires the image to be bound as the current read framebuffer
// For pick buffer, we need to read after the pick pass while FBO is still bound

#include <string.h>

static inline bool pick_readback_pixels(sg_image img, int width, int height, uint8_t *pixel_data) {
    (void)img;  // Not used directly - we read from current FBO

    // Note: This is a placeholder. Proper implementation requires:
    // 1. Binding the image's FBO
    // 2. Calling glReadPixels
    // For now, clear to zero (no hover)
    memset(pixel_data, 0, width * height * 4);
    return false;  // Indicate readback not implemented
}

#elif defined(SOKOL_WGPU)

// WebGPU implementation - requires async buffer mapping
// Not yet implemented

#include <string.h>

static inline bool pick_readback_pixels(sg_image img, int width, int height, uint8_t *pixel_data) {
    (void)img;
    memset(pixel_data, 0, width * height * 4);
    return false;  // Indicate readback not implemented
}

#elif defined(SOKOL_D3D11)

// D3D11 implementation - requires staging texture
// Not yet implemented

#include <string.h>

static inline bool pick_readback_pixels(sg_image img, int width, int height, uint8_t *pixel_data) {
    (void)img;
    memset(pixel_data, 0, width * height * 4);
    return false;  // Indicate readback not implemented
}

#else

// Fallback - no readback support
#include <string.h>

static inline bool pick_readback_pixels(sg_image img, int width, int height, uint8_t *pixel_data) {
    (void)img;
    memset(pixel_data, 0, width * height * 4);
    return false;
}

#endif

#endif // PICK_READBACK_H
