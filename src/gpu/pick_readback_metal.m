//------------------------------------------------------------------------------
// pick_readback_metal.m - Metal-specific GPU texture readback
//
// Uses MTLTexture getBytes to read pixel data from GPU to CPU.
//------------------------------------------------------------------------------

#include "../platform.h"

#if defined(SOKOL_METAL)

#import <Metal/Metal.h>
#include "sokol_gfx.h"
#include <stdint.h>
#include <stdbool.h>

bool pick_readback_pixels(sg_image img, int width, int height, uint8_t *pixel_data) {
    if (!pixel_data || width <= 0 || height <= 0) {
        return false;
    }

    // Get the Metal texture info from Sokol
    sg_mtl_image_info info = sg_mtl_query_image_info(img);

    // Get the active texture (Sokol uses triple buffering)
    id<MTLTexture> texture = (__bridge id<MTLTexture>)info.tex[info.active_slot];

    if (!texture) {
        return false;
    }

    // Define the region to read (entire texture)
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);

    // Calculate bytes per row (RGBA8 = 4 bytes per pixel)
    NSUInteger bytesPerRow = width * 4;

    // Read the texture data into CPU buffer
    // Note: This is a synchronous operation that may stall the GPU pipeline
    // For production, consider using a staging buffer with async readback
    [texture getBytes:pixel_data
          bytesPerRow:bytesPerRow
           fromRegion:region
          mipmapLevel:0];

    return true;
}

#endif // SOKOL_METAL
