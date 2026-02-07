//------------------------------------------------------------------------------
// pick_readback_opengl.c - OpenGL-specific GPU texture readback
//
// Uses glReadPixels with a framebuffer to read pixel data from GPU to CPU.
// Note: OpenGL framebuffers are Y-flipped compared to Metal/D3D, so we flip
// the rows during readback to maintain consistent behavior across backends.
//------------------------------------------------------------------------------

#include "../platform.h"

#if defined(SOKOL_GLCORE) || defined(SOKOL_GLES3)

#include "sokol_gfx.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

// OpenGL headers
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

#if defined(SOKOL_GLCORE)
    // Desktop OpenGL
    #ifdef _WIN32
        #include <GL/gl.h>
        // OpenGL extension function pointers (loaded by Sokol or window system)
        typedef void (APIENTRY *PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
        typedef void (APIENTRY *PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint *framebuffers);
        typedef void (APIENTRY *PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n, const GLuint *framebuffers);
        typedef void (APIENTRY *PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);

        #define GL_FRAMEBUFFER 0x8D40
        #define GL_COLOR_ATTACHMENT0 0x8CE0
    #else
        #include <GL/gl.h>
        #include <GL/glext.h>
    #endif
#else
    // OpenGL ES 3.0
    #include <GLES3/gl3.h>
#endif

bool pick_readback_pixels(sg_image img, int width, int height, uint8_t *pixel_data) {
    if (!pixel_data || width <= 0 || height <= 0) {
        return false;
    }

    // Get the OpenGL texture handle from Sokol
    sg_gl_image_info info = sg_gl_query_image_info(img);
    GLuint texture = info.tex[info.active_slot];

    if (texture == 0) {
        return false;
    }

    // We need to bind the texture to a framebuffer to read from it
    // Get OpenGL function pointers (these should be available via Sokol's loader)
#ifdef _WIN32
    static PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = NULL;
    static PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = NULL;
    static PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = NULL;
    static PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = NULL;

    if (!glGenFramebuffers) {
        glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
        glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress("glDeleteFramebuffers");
        glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
        glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");

        if (!glGenFramebuffers || !glDeleteFramebuffers || !glBindFramebuffer || !glFramebufferTexture2D) {
            // OpenGL extensions not available
            memset(pixel_data, 0, (size_t)width * (size_t)height * 4);
            return false;
        }
    }
#endif

    // Create a temporary framebuffer and attach our texture
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // Allocate temporary buffer for Y-flipped read
    uint8_t *temp_data = (uint8_t*)malloc((size_t)width * (size_t)height * 4);
    if (!temp_data) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);
        return false;
    }

    // Read pixels from framebuffer
    // OpenGL stores framebuffer with Y=0 at bottom, so we read and then flip
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, temp_data);

    // Flip rows to match Metal/D3D coordinate system (Y=0 at top)
    int row_size = width * 4;
    for (int y = 0; y < height; y++) {
        int src_row = height - 1 - y;  // Read from bottom
        memcpy(pixel_data + y * row_size, temp_data + src_row * row_size, row_size);
    }

    free(temp_data);

    // Cleanup
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);

    return true;
}

#endif // SOKOL_GLCORE || SOKOL_GLES3
