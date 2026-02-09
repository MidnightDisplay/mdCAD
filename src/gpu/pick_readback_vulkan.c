//------------------------------------------------------------------------------
// pick_readback_vulkan.c - Vulkan-specific GPU texture readback
//
// Uses a staging buffer to read pixel data from GPU to CPU for pick buffer.
// This implementation uses a custom Sokol extension (sg_vk_query_image_info_ext)
// that is defined in vendors/libsokol/sokol.c.
//------------------------------------------------------------------------------

#include "../platform.h"

#if defined(SOKOL_VULKAN)

#include <vulkan/vulkan.h>
#include "sokol_gfx.h"
#include "sokol_app.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

//------------------------------------------------------------------------------
// External: Vulkan image query extension (defined in sokol.c)
//------------------------------------------------------------------------------
typedef struct sg_vk_image_info_ext {
    VkImage image;
    VkDeviceMemory memory;
} sg_vk_image_info_ext;

extern sg_vk_image_info_ext sg_vk_query_image_info_ext(sg_image img_id);

//------------------------------------------------------------------------------
// Vulkan pick readback implementation
//------------------------------------------------------------------------------

// Find memory type index that supports the given properties
static uint32_t vk_find_memory_type(VkPhysicalDevice phys_dev, uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_props);
    
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && 
            (mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return UINT32_MAX;
}

bool pick_readback_pixels(sg_image img, int width, int height, uint8_t *pixel_data) {
    if (!pixel_data || width <= 0 || height <= 0) {
        return false;
    }

    // Get Vulkan handles from sokol_app
    sapp_environment env = sapp_get_environment();
    VkDevice device = (VkDevice)env.vulkan.device;
    VkPhysicalDevice phys_dev = (VkPhysicalDevice)env.vulkan.physical_device;
    VkQueue queue = (VkQueue)env.vulkan.queue;
    uint32_t queue_family = env.vulkan.queue_family_index;
    
    if (!device || !phys_dev || !queue) {
        memset(pixel_data, 0, (size_t)width * (size_t)height * 4);
        return false;
    }

    // Get VkImage from Sokol image handle using our custom extension
    sg_vk_image_info_ext img_info = sg_vk_query_image_info_ext(img);
    VkImage src_image = img_info.image;
    
    if (!src_image) {
        memset(pixel_data, 0, (size_t)width * (size_t)height * 4);
        return false;
    }

    VkResult result;
    bool success = false;
    
    // Calculate buffer size (RGBA8 = 4 bytes per pixel)
    VkDeviceSize buffer_size = (VkDeviceSize)width * (VkDeviceSize)height * 4;
    
    // Create staging buffer
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;
    
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = buffer_size,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    
    result = vkCreateBuffer(device, &buffer_info, NULL, &staging_buffer);
    if (result != VK_SUCCESS) {
        goto cleanup;
    }
    
    // Get memory requirements and allocate
    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device, staging_buffer, &mem_reqs);
    
    uint32_t mem_type = vk_find_memory_type(phys_dev, mem_reqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (mem_type == UINT32_MAX) {
        goto cleanup;
    }
    
    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = mem_type,
    };
    
    result = vkAllocateMemory(device, &alloc_info, NULL, &staging_memory);
    if (result != VK_SUCCESS) {
        goto cleanup;
    }
    
    result = vkBindBufferMemory(device, staging_buffer, staging_memory, 0);
    if (result != VK_SUCCESS) {
        goto cleanup;
    }
    
    // Create command pool and buffer
    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    VkCommandBuffer cmd_buf = VK_NULL_HANDLE;
    
    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = queue_family,
    };
    
    result = vkCreateCommandPool(device, &pool_info, NULL, &cmd_pool);
    if (result != VK_SUCCESS) {
        goto cleanup;
    }
    
    VkCommandBufferAllocateInfo cmd_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    
    result = vkAllocateCommandBuffers(device, &cmd_alloc_info, &cmd_buf);
    if (result != VK_SUCCESS) {
        goto cleanup_pool;
    }
    
    // Begin command buffer
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    
    result = vkBeginCommandBuffer(cmd_buf, &begin_info);
    if (result != VK_SUCCESS) {
        goto cleanup_pool;
    }
    
    // Transition image layout to TRANSFER_SRC_OPTIMAL
    // Note: Sokol uses VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL for color attachments
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = src_image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    
    vkCmdPipelineBarrier(cmd_buf,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, NULL, 0, NULL, 1, &barrier);
    
    // Copy image to buffer
    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,  // Tightly packed
        .bufferImageHeight = 0, // Tightly packed
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {(uint32_t)width, (uint32_t)height, 1},
    };
    
    vkCmdCopyImageToBuffer(cmd_buf, src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        staging_buffer, 1, &region);
    
    // Transition image back to ATTACHMENT_OPTIMAL (Sokol's format)
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    
    vkCmdPipelineBarrier(cmd_buf,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0, 0, NULL, 0, NULL, 1, &barrier);
    
    // End and submit command buffer
    result = vkEndCommandBuffer(cmd_buf);
    if (result != VK_SUCCESS) {
        goto cleanup_pool;
    }
    
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd_buf,
    };
    
    result = vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
        goto cleanup_pool;
    }
    
    // Wait for transfer to complete
    result = vkQueueWaitIdle(queue);
    if (result != VK_SUCCESS) {
        goto cleanup_pool;
    }
    
    // Map staging buffer and copy to output
    void* mapped_data = NULL;
    result = vkMapMemory(device, staging_memory, 0, buffer_size, 0, &mapped_data);
    if (result == VK_SUCCESS && mapped_data) {
        memcpy(pixel_data, mapped_data, (size_t)buffer_size);
        vkUnmapMemory(device, staging_memory);
        success = true;
    }
    
cleanup_pool:
    if (cmd_pool) {
        vkDestroyCommandPool(device, cmd_pool, NULL);
    }
    
cleanup:
    if (staging_buffer) {
        vkDestroyBuffer(device, staging_buffer, NULL);
    }
    if (staging_memory) {
        vkFreeMemory(device, staging_memory, NULL);
    }
    
    if (!success) {
        memset(pixel_data, 0, (size_t)width * (size_t)height * 4);
    }
    
    return success;
}

#endif // SOKOL_VULKAN
