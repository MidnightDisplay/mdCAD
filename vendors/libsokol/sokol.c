// sokol implementation library on non-Apple platforms
#define SOKOL_IMPL

// Platform detection (shared with src/app.c)
#include "../../src/platform.h"

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include "sokol_fetch.h"
#include "sokol_time.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#define SOKOL_IMGUI_IMPL
#include "sokol_imgui.h"

//------------------------------------------------------------------------------
// Custom Vulkan query extension (not provided by Sokol)
//
// Sokol doesn't expose sg_vk_query_image_info() like it does for Metal/D3D11.
// This is a local extension that accesses Sokol internals to get the VkImage
// handle for GPU picking readback.
//
// WARNING: This depends on Sokol's internal structure layout and may break
// with future Sokol updates. If Sokol adds official sg_vk_query_image_info(),
// this code should be removed and replaced with the official API.
//------------------------------------------------------------------------------
#if defined(SOKOL_VULKAN)

#include <vulkan/vulkan.h>

// Struct to return Vulkan image info (mirrors pattern from sg_d3d11_image_info)
typedef struct sg_vk_image_info_ext {
    VkImage image;
    VkDeviceMemory memory;
} sg_vk_image_info_ext;

// Query Vulkan image handles from sg_image
// Returns zeroed struct if image is invalid
sg_vk_image_info_ext sg_vk_query_image_info_ext(sg_image img_id) {
    sg_vk_image_info_ext res = {0};
    
    // _sg is defined in sokol_gfx.h when SOKOL_IMPL is set (this file)
    if (_sg.valid && (img_id.id != SG_INVALID_ID)) {
        // Use Sokol's internal slot index calculation
        int slot_index = (int)(img_id.id & 0xFFFF);
        if (slot_index > 0 && slot_index < _sg.pools.image_pool.size) {
            const _sg_image_t* img = &_sg.pools.images[slot_index];
            if (img->slot.id == img_id.id) {
                res.image = img->vk.img;
                res.memory = img->vk.mem;
            }
        }
    }
    
    return res;
}

#endif // SOKOL_VULKAN
