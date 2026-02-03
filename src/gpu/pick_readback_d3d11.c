//------------------------------------------------------------------------------
// pick_readback_d3d11.c - D3D11-specific GPU texture readback
//
// Uses a staging texture to read pixel data from GPU to CPU.
//------------------------------------------------------------------------------

#include "../platform.h"

#if defined(SOKOL_D3D11)

#define COBJMACROS
#include <d3d11.h>
#include "sokol_gfx.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

bool pick_readback_pixels(sg_image img, int width, int height, uint8_t *pixel_data) {
    if (!pixel_data || width <= 0 || height <= 0) {
        return false;
    }

    // Get D3D11 device and context from Sokol
    ID3D11Device* device = (ID3D11Device*)sg_d3d11_device();
    ID3D11DeviceContext* context = (ID3D11DeviceContext*)sg_d3d11_device_context();

    if (!device || !context) {
        return false;
    }

    // Get the D3D11 texture from Sokol
    sg_d3d11_image_info info = sg_d3d11_query_image_info(img);
    ID3D11Texture2D* src_texture = (ID3D11Texture2D*)info.tex2d;

    if (!src_texture) {
        return false;
    }

    // Create a staging texture for CPU readback
    D3D11_TEXTURE2D_DESC desc;
    ID3D11Texture2D_GetDesc(src_texture, &desc);

    D3D11_TEXTURE2D_DESC staging_desc = {
        .Width = (UINT)width,
        .Height = (UINT)height,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,  // RGBA8
        .SampleDesc = { .Count = 1, .Quality = 0 },
        .Usage = D3D11_USAGE_STAGING,
        .BindFlags = 0,
        .CPUAccessFlags = D3D11_CPU_ACCESS_READ,
        .MiscFlags = 0
    };

    ID3D11Texture2D* staging_texture = NULL;
    HRESULT hr = ID3D11Device_CreateTexture2D(device, &staging_desc, NULL, &staging_texture);
    if (FAILED(hr) || !staging_texture) {
        return false;
    }

    // Copy from source texture to staging texture
    ID3D11DeviceContext_CopyResource(context, (ID3D11Resource*)staging_texture, (ID3D11Resource*)src_texture);

    // Map the staging texture for CPU read
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = ID3D11DeviceContext_Map(context, (ID3D11Resource*)staging_texture, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        ID3D11Texture2D_Release(staging_texture);
        return false;
    }

    // Copy pixel data row by row (account for row pitch padding)
    const uint8_t* src = (const uint8_t*)mapped.pData;
    int dst_row_size = width * 4;  // RGBA8 = 4 bytes per pixel

    for (int y = 0; y < height; y++) {
        memcpy(pixel_data + y * dst_row_size, src + y * mapped.RowPitch, dst_row_size);
    }

    // Unmap and release staging texture
    ID3D11DeviceContext_Unmap(context, (ID3D11Resource*)staging_texture, 0);
    ID3D11Texture2D_Release(staging_texture);

    return true;
}

#endif // SOKOL_D3D11
