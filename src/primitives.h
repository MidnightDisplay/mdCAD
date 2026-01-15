//------------------------------------------------------------------------------
// primitives.h - Mesh types and primitive geometry data (header-only)
//------------------------------------------------------------------------------
#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include <stdint.h>

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
typedef struct {
    float px, py, pz;   // position
    float nx, ny, nz;   // normal
} vertex_t;

typedef struct {
    const vertex_t* vertices;
    int vertex_count;
    const uint16_t* indices;
    int index_count;
} mesh_t;

//------------------------------------------------------------------------------
// Cube primitive - centered at origin, size 1.0
//------------------------------------------------------------------------------
static const vertex_t cube_vertices[] = {
    // Front face (z = +0.5), normal (0, 0, 1)
    { -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f },
    {  0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f },
    {  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f },
    { -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f },
    // Back face (z = -0.5), normal (0, 0, -1)
    {  0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f },
    { -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f },
    { -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f },
    {  0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f },
    // Right face (x = +0.5), normal (1, 0, 0)
    {  0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f },
    {  0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f },
    {  0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f },
    {  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f },
    // Left face (x = -0.5), normal (-1, 0, 0)
    { -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f },
    { -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f },
    { -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f },
    { -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f },
    // Top face (y = +0.5), normal (0, 1, 0)
    { -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f },
    {  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f },
    {  0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f },
    { -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f },
    // Bottom face (y = -0.5), normal (0, -1, 0)
    { -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f },
    {  0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f },
    {  0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f },
    { -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f },
};

static const uint16_t cube_indices[] = {
    // CCW winding when viewed from outside (for correct backface culling)
    0,  2,  1,   0,  3,  2,   // front
    4,  6,  5,   4,  7,  6,   // back
    8,  10, 9,   8,  11, 10,  // right
    12, 14, 13,  12, 15, 14,  // left
    16, 18, 17,  16, 19, 18,  // top
    20, 22, 21,  20, 23, 22,  // bottom
};

//------------------------------------------------------------------------------
// Mesh factory functions
//------------------------------------------------------------------------------
static inline mesh_t mesh_cube(void) {
    return (mesh_t){
        .vertices = cube_vertices,
        .vertex_count = sizeof(cube_vertices) / sizeof(cube_vertices[0]),
        .indices = cube_indices,
        .index_count = sizeof(cube_indices) / sizeof(cube_indices[0])
    };
}

#endif // PRIMITIVES_H
