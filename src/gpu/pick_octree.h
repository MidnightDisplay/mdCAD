//------------------------------------------------------------------------------
// pick_octree.h - Loose octree spatial index for pick buffer culling
//
// Replaces O(N) entity iteration with O(log N + K) frustum query where
// K is the number of entities whose world-space AABBs overlap the pick frustum.
//------------------------------------------------------------------------------
#ifndef PICK_OCTREE_H
#define PICK_OCTREE_H

#include "../math3d.h"
#include "../components/component_types.h"
#include "../components/geometry_comp.h"
#include "../components/transform_comp.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------
#define PICK_OCTREE_MAX_DEPTH       20
#define PICK_OCTREE_LEAF_CAPACITY   64
#define PICK_OCTREE_INITIAL_NODES   4096
#define PICK_OCTREE_INITIAL_ENTRIES 8192
#define PICK_OCTREE_QUERY_STACK     256
#define PICK_OCTREE_NULL            0xFFFFFFFF
#define PICK_OCTREE_AABB_EPSILON    0.001f

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct { vec3_t min, max; } pick_aabb_t;

typedef struct {
    uint64_t entity;        // ecs_entity_t
    pick_aabb_t aabb;       // World-space AABB
    uint32_t node_idx;      // Containing node (for O(1) removal)
    uint32_t next;          // Next entry in same node, or PICK_OCTREE_NULL
} pick_octree_entry_t;

typedef struct {
    vec3_t center;          // Center of tight region
    float half_size;        // Half-size of tight region (loose = 2x)
    uint32_t children[8];   // Child indices, or PICK_OCTREE_NULL
    uint32_t first_entry;   // Head of entity linked list
    uint16_t entry_count;   // Entities in this node
    uint16_t depth;
    bool is_leaf;
} pick_octree_node_t;

typedef struct { float a, b, c, d; } pick_frustum_plane_t;
typedef struct { pick_frustum_plane_t planes[6]; } pick_frustum_t;

typedef struct {
    pick_octree_node_t *nodes;
    uint32_t node_count, node_capacity;
    pick_octree_entry_t *entries;
    uint32_t entry_count, entry_capacity;
    uint32_t *free_nodes;
    uint32_t free_node_count, free_node_capacity;
    uint32_t *free_entries;
    uint32_t free_entry_count, free_entry_capacity;
    uint32_t *pick_id_to_entry;     // pick_id -> entry index (O(1) lookup)
    uint32_t pick_id_map_capacity;
    uint64_t *query_results;
    uint32_t query_result_count, query_result_capacity;
    uint32_t root;
    bool initialized;
} pick_octree_t;

//------------------------------------------------------------------------------
// AABB helpers
//------------------------------------------------------------------------------

static inline pick_aabb_t pick_aabb_empty(void) {
    return (pick_aabb_t){
        .min = vec3_make(1e30f, 1e30f, 1e30f),
        .max = vec3_make(-1e30f, -1e30f, -1e30f)
    };
}

static inline void pick_aabb_expand_point(pick_aabb_t *aabb, vec3_t p) {
    if (p.x < aabb->min.x) aabb->min.x = p.x;
    if (p.y < aabb->min.y) aabb->min.y = p.y;
    if (p.z < aabb->min.z) aabb->min.z = p.z;
    if (p.x > aabb->max.x) aabb->max.x = p.x;
    if (p.y > aabb->max.y) aabb->max.y = p.y;
    if (p.z > aabb->max.z) aabb->max.z = p.z;
}

static inline void pick_aabb_pad(pick_aabb_t *aabb, float pad) {
    aabb->min.x -= pad; aabb->min.y -= pad; aabb->min.z -= pad;
    aabb->max.x += pad; aabb->max.y += pad; aabb->max.z += pad;
}

static inline vec3_t pick_aabb_center(pick_aabb_t aabb) {
    return vec3_make(
        (aabb.min.x + aabb.max.x) * 0.5f,
        (aabb.min.y + aabb.max.y) * 0.5f,
        (aabb.min.z + aabb.max.z) * 0.5f
    );
}

static inline float pick_aabb_max_extent(pick_aabb_t aabb) {
    float ex = aabb.max.x - aabb.min.x;
    float ey = aabb.max.y - aabb.min.y;
    float ez = aabb.max.z - aabb.min.z;
    float m = ex;
    if (ey > m) m = ey;
    if (ez > m) m = ez;
    return m;
}

//------------------------------------------------------------------------------
// Entity AABB computation per geometry type
//------------------------------------------------------------------------------

static inline pick_aabb_t pick_octree_compute_entity_aabb(
    const GeometryComp *g, const TransformComp *t)
{
    pick_aabb_t aabb = pick_aabb_empty();

    switch (g->type) {
        case GEOM_POINT: {
            vec3_t wp = mat4_transform_point(t->world_matrix, g->data.point.point);
            pick_aabb_expand_point(&aabb, wp);
            pick_aabb_pad(&aabb, PICK_OCTREE_AABB_EPSILON);
            break;
        }
        case GEOM_LINE: {
            vec3_t wa = mat4_transform_point(t->world_matrix, g->data.line.a);
            vec3_t wb = mat4_transform_point(t->world_matrix, g->data.line.b);
            pick_aabb_expand_point(&aabb, wa);
            pick_aabb_expand_point(&aabb, wb);
            break;
        }
        case GEOM_TRIANGLE: {
            vec3_t wa = mat4_transform_point(t->world_matrix, g->data.triangle.a);
            vec3_t wb = mat4_transform_point(t->world_matrix, g->data.triangle.b);
            vec3_t wc = mat4_transform_point(t->world_matrix, g->data.triangle.c);
            pick_aabb_expand_point(&aabb, wa);
            pick_aabb_expand_point(&aabb, wb);
            pick_aabb_expand_point(&aabb, wc);
            break;
        }
        case GEOM_POLYLINE: {
            for (int i = 0; i < g->data.polyline.count; i++) {
                vec3_t wp = mat4_transform_point(t->world_matrix, g->data.polyline.points[i]);
                pick_aabb_expand_point(&aabb, wp);
            }
            break;
        }
        case GEOM_POLYGON: {
            for (int i = 0; i < g->data.polygon.count; i++) {
                vec3_t wp = mat4_transform_point(t->world_matrix, g->data.polygon.points[i]);
                pick_aabb_expand_point(&aabb, wp);
            }
            break;
        }
        case GEOM_ARC: {
            // Conservative: center +/- radius sphere bound
            vec3_t wc = mat4_transform_point(t->world_matrix, g->data.arc.center);
            float r = g->data.arc.radius;
            aabb.min = vec3_make(wc.x - r, wc.y - r, wc.z - r);
            aabb.max = vec3_make(wc.x + r, wc.y + r, wc.z + r);
            break;
        }
        case GEOM_BEZIER: {
            // Conservative: AABB of 4 control points (convex hull property)
            vec3_t wp0 = mat4_transform_point(t->world_matrix, g->data.bezier.p0);
            vec3_t wp1 = mat4_transform_point(t->world_matrix, g->data.bezier.p1);
            vec3_t wp2 = mat4_transform_point(t->world_matrix, g->data.bezier.p2);
            vec3_t wp3 = mat4_transform_point(t->world_matrix, g->data.bezier.p3);
            pick_aabb_expand_point(&aabb, wp0);
            pick_aabb_expand_point(&aabb, wp1);
            pick_aabb_expand_point(&aabb, wp2);
            pick_aabb_expand_point(&aabb, wp3);
            break;
        }
        case GEOM_HELIX: {
            // Conservative: AABB of axis endpoints +/- radius
            vec3_t ws = mat4_transform_point(t->world_matrix, g->data.helix.axis_start);
            vec3_t we = mat4_transform_point(t->world_matrix, g->data.helix.axis_end);
            float r = g->data.helix.radius;
            pick_aabb_expand_point(&aabb, ws);
            pick_aabb_expand_point(&aabb, we);
            pick_aabb_pad(&aabb, r);
            break;
        }
        case GEOM_POINT_CLOUD: {
            for (int i = 0; i < g->data.point_cloud.count; i++) {
                vec3_t wp = mat4_transform_point(t->world_matrix, g->data.point_cloud.points[i]);
                pick_aabb_expand_point(&aabb, wp);
            }
            pick_aabb_pad(&aabb, PICK_OCTREE_AABB_EPSILON);
            break;
        }
        case GEOM_MESH: {
            for (int i = 0; i < g->data.mesh.vertex_count; i++) {
                vec3_t wp = mat4_transform_point(t->world_matrix, g->data.mesh.vertices[i]);
                pick_aabb_expand_point(&aabb, wp);
            }
            break;
        }
        default:
            aabb.min = vec3_make(0, 0, 0);
            aabb.max = vec3_make(0, 0, 0);
            break;
    }

    return aabb;
}

//------------------------------------------------------------------------------
// Frustum extraction (Gribb-Hartmann method)
//------------------------------------------------------------------------------

static inline pick_frustum_t pick_octree_extract_frustum(mat4_t m) {
    pick_frustum_t f;
    // Column-major: m[col*4 + row], so row i = m[0*4+i], m[1*4+i], m[2*4+i], m[3*4+i]
    // Row 0: m[0], m[4], m[8],  m[12]
    // Row 1: m[1], m[5], m[9],  m[13]
    // Row 2: m[2], m[6], m[10], m[14]
    // Row 3: m[3], m[7], m[11], m[15]

    // Left: row3 + row0
    f.planes[0].a = m.m[3]  + m.m[0];
    f.planes[0].b = m.m[7]  + m.m[4];
    f.planes[0].c = m.m[11] + m.m[8];
    f.planes[0].d = m.m[15] + m.m[12];

    // Right: row3 - row0
    f.planes[1].a = m.m[3]  - m.m[0];
    f.planes[1].b = m.m[7]  - m.m[4];
    f.planes[1].c = m.m[11] - m.m[8];
    f.planes[1].d = m.m[15] - m.m[12];

    // Bottom: row3 + row1
    f.planes[2].a = m.m[3]  + m.m[1];
    f.planes[2].b = m.m[7]  + m.m[5];
    f.planes[2].c = m.m[11] + m.m[9];
    f.planes[2].d = m.m[15] + m.m[13];

    // Top: row3 - row1
    f.planes[3].a = m.m[3]  - m.m[1];
    f.planes[3].b = m.m[7]  - m.m[5];
    f.planes[3].c = m.m[11] - m.m[9];
    f.planes[3].d = m.m[15] - m.m[13];

    // Near: row3 + row2
    f.planes[4].a = m.m[3]  + m.m[2];
    f.planes[4].b = m.m[7]  + m.m[6];
    f.planes[4].c = m.m[11] + m.m[10];
    f.planes[4].d = m.m[15] + m.m[14];

    // Far: row3 - row2
    f.planes[5].a = m.m[3]  - m.m[2];
    f.planes[5].b = m.m[7]  - m.m[6];
    f.planes[5].c = m.m[11] - m.m[10];
    f.planes[5].d = m.m[15] - m.m[14];

    // Normalize each plane
    for (int i = 0; i < 6; i++) {
        float len = sqrtf(f.planes[i].a * f.planes[i].a +
                          f.planes[i].b * f.planes[i].b +
                          f.planes[i].c * f.planes[i].c);
        if (len > 0.0f) {
            float inv = 1.0f / len;
            f.planes[i].a *= inv;
            f.planes[i].b *= inv;
            f.planes[i].c *= inv;
            f.planes[i].d *= inv;
        }
    }

    return f;
}

//------------------------------------------------------------------------------
// AABB-Frustum test (positive vertex method)
//------------------------------------------------------------------------------

static inline bool pick_aabb_in_frustum(pick_aabb_t aabb, const pick_frustum_t *f) {
    for (int i = 0; i < 6; i++) {
        // Compute positive vertex (p-vertex): the corner most aligned with the plane normal
        float px = (f->planes[i].a >= 0.0f) ? aabb.max.x : aabb.min.x;
        float py = (f->planes[i].b >= 0.0f) ? aabb.max.y : aabb.min.y;
        float pz = (f->planes[i].c >= 0.0f) ? aabb.max.z : aabb.min.z;

        float dist = f->planes[i].a * px + f->planes[i].b * py +
                     f->planes[i].c * pz + f->planes[i].d;
        if (dist < 0.0f) return false; // Entirely outside this plane
    }
    return true; // Conservatively inside
}

//------------------------------------------------------------------------------
// Pool management
//------------------------------------------------------------------------------

static inline uint32_t pick_octree_alloc_node(pick_octree_t *tree) {
    if (tree->free_node_count > 0) {
        return tree->free_nodes[--tree->free_node_count];
    }
    if (tree->node_count >= tree->node_capacity) {
        tree->node_capacity = tree->node_capacity * 2;
        tree->nodes = (pick_octree_node_t*)realloc(tree->nodes,
            tree->node_capacity * sizeof(pick_octree_node_t));
    }
    return tree->node_count++;
}

static inline void pick_octree_free_node(pick_octree_t *tree, uint32_t idx) {
    if (tree->free_node_count >= tree->free_node_capacity) {
        tree->free_node_capacity = tree->free_node_capacity * 2;
        tree->free_nodes = (uint32_t*)realloc(tree->free_nodes,
            tree->free_node_capacity * sizeof(uint32_t));
    }
    tree->free_nodes[tree->free_node_count++] = idx;
}

static inline uint32_t pick_octree_alloc_entry(pick_octree_t *tree) {
    if (tree->free_entry_count > 0) {
        return tree->free_entries[--tree->free_entry_count];
    }
    if (tree->entry_count >= tree->entry_capacity) {
        tree->entry_capacity = tree->entry_capacity * 2;
        tree->entries = (pick_octree_entry_t*)realloc(tree->entries,
            tree->entry_capacity * sizeof(pick_octree_entry_t));
    }
    return tree->entry_count++;
}

static inline void pick_octree_free_entry(pick_octree_t *tree, uint32_t idx) {
    if (tree->free_entry_count >= tree->free_entry_capacity) {
        tree->free_entry_capacity = tree->free_entry_capacity * 2;
        tree->free_entries = (uint32_t*)realloc(tree->free_entries,
            tree->free_entry_capacity * sizeof(uint32_t));
    }
    tree->free_entries[tree->free_entry_count++] = idx;
}

//------------------------------------------------------------------------------
// pick_id -> entry mapping
//------------------------------------------------------------------------------

static inline void pick_octree_map_ensure(pick_octree_t *tree, uint32_t pick_id) {
    if (pick_id < tree->pick_id_map_capacity) return;
    uint32_t new_cap = tree->pick_id_map_capacity;
    if (new_cap == 0) new_cap = 1024;
    while (new_cap <= pick_id) new_cap *= 2;
    tree->pick_id_to_entry = (uint32_t*)realloc(tree->pick_id_to_entry,
        new_cap * sizeof(uint32_t));
    for (uint32_t i = tree->pick_id_map_capacity; i < new_cap; i++) {
        tree->pick_id_to_entry[i] = PICK_OCTREE_NULL;
    }
    tree->pick_id_map_capacity = new_cap;
}

//------------------------------------------------------------------------------
// Init / Shutdown
//------------------------------------------------------------------------------

static inline void pick_octree_init(pick_octree_t *tree, vec3_t center, float half_size) {
    memset(tree, 0, sizeof(*tree));

    tree->node_capacity = PICK_OCTREE_INITIAL_NODES;
    tree->nodes = (pick_octree_node_t*)malloc(tree->node_capacity * sizeof(pick_octree_node_t));
    tree->node_count = 0;

    tree->entry_capacity = PICK_OCTREE_INITIAL_ENTRIES;
    tree->entries = (pick_octree_entry_t*)malloc(tree->entry_capacity * sizeof(pick_octree_entry_t));
    tree->entry_count = 0;

    tree->free_node_capacity = 256;
    tree->free_nodes = (uint32_t*)malloc(tree->free_node_capacity * sizeof(uint32_t));
    tree->free_node_count = 0;

    tree->free_entry_capacity = 256;
    tree->free_entries = (uint32_t*)malloc(tree->free_entry_capacity * sizeof(uint32_t));
    tree->free_entry_count = 0;

    tree->pick_id_map_capacity = 1024;
    tree->pick_id_to_entry = (uint32_t*)malloc(tree->pick_id_map_capacity * sizeof(uint32_t));
    for (uint32_t i = 0; i < tree->pick_id_map_capacity; i++) {
        tree->pick_id_to_entry[i] = PICK_OCTREE_NULL;
    }

    tree->query_result_capacity = 256;
    tree->query_results = (uint64_t*)malloc(tree->query_result_capacity * sizeof(uint64_t));
    tree->query_result_count = 0;

    // Create root node
    tree->root = pick_octree_alloc_node(tree);
    pick_octree_node_t *root = &tree->nodes[tree->root];
    root->center = center;
    root->half_size = half_size;
    for (int i = 0; i < 8; i++) root->children[i] = PICK_OCTREE_NULL;
    root->first_entry = PICK_OCTREE_NULL;
    root->entry_count = 0;
    root->depth = 0;
    root->is_leaf = true;

    tree->initialized = true;
}

static inline void pick_octree_shutdown(pick_octree_t *tree) {
    if (!tree->initialized) return;
    free(tree->nodes);
    free(tree->entries);
    free(tree->free_nodes);
    free(tree->free_entries);
    free(tree->pick_id_to_entry);
    free(tree->query_results);
    memset(tree, 0, sizeof(*tree));
}

//------------------------------------------------------------------------------
// Child octant selection
//------------------------------------------------------------------------------

static inline int pick_octree_octant(vec3_t node_center, vec3_t point) {
    int octant = 0;
    if (point.x >= node_center.x) octant |= 1;
    if (point.y >= node_center.y) octant |= 2;
    if (point.z >= node_center.z) octant |= 4;
    return octant;
}

static inline vec3_t pick_octree_child_center(vec3_t parent_center, float parent_half, int octant) {
    float q = parent_half * 0.5f;
    return vec3_make(
        parent_center.x + ((octant & 1) ? q : -q),
        parent_center.y + ((octant & 2) ? q : -q),
        parent_center.z + ((octant & 4) ? q : -q)
    );
}

//------------------------------------------------------------------------------
// Insert entry into a node's linked list
//------------------------------------------------------------------------------

static inline void pick_octree_link_entry(pick_octree_t *tree, uint32_t node_idx, uint32_t entry_idx) {
    pick_octree_node_t *node = &tree->nodes[node_idx];
    pick_octree_entry_t *entry = &tree->entries[entry_idx];
    entry->node_idx = node_idx;
    entry->next = node->first_entry;
    node->first_entry = entry_idx;
    node->entry_count++;
}

//------------------------------------------------------------------------------
// Unlink entry from a node's linked list
//------------------------------------------------------------------------------

static inline void pick_octree_unlink_entry(pick_octree_t *tree, uint32_t node_idx, uint32_t entry_idx) {
    pick_octree_node_t *node = &tree->nodes[node_idx];

    if (node->first_entry == entry_idx) {
        node->first_entry = tree->entries[entry_idx].next;
    } else {
        uint32_t prev = node->first_entry;
        while (prev != PICK_OCTREE_NULL && tree->entries[prev].next != entry_idx) {
            prev = tree->entries[prev].next;
        }
        if (prev != PICK_OCTREE_NULL) {
            tree->entries[prev].next = tree->entries[entry_idx].next;
        }
    }
    node->entry_count--;
}

//------------------------------------------------------------------------------
// Split a leaf node into 8 children and redistribute entries
//------------------------------------------------------------------------------

static inline void pick_octree_split(pick_octree_t *tree, uint32_t node_idx) {
    pick_octree_node_t *node = &tree->nodes[node_idx];
    if (!node->is_leaf) return;

    float child_half = node->half_size * 0.5f;
    uint16_t child_depth = node->depth + 1;

    // Allocate 8 children
    for (int i = 0; i < 8; i++) {
        uint32_t ci = pick_octree_alloc_node(tree);
        // Re-fetch node pointer since alloc may realloc
        node = &tree->nodes[node_idx];
        node->children[i] = ci;

        pick_octree_node_t *child = &tree->nodes[ci];
        child->center = pick_octree_child_center(node->center, node->half_size, i);
        child->half_size = child_half;
        for (int j = 0; j < 8; j++) child->children[j] = PICK_OCTREE_NULL;
        child->first_entry = PICK_OCTREE_NULL;
        child->entry_count = 0;
        child->depth = child_depth;
        child->is_leaf = true;
    }

    node = &tree->nodes[node_idx];
    node->is_leaf = false;

    // Redistribute entries
    uint32_t cur = node->first_entry;
    node->first_entry = PICK_OCTREE_NULL;
    node->entry_count = 0;

    while (cur != PICK_OCTREE_NULL) {
        uint32_t next = tree->entries[cur].next;
        pick_aabb_t *eaabb = &tree->entries[cur].aabb;
        vec3_t ecenter = pick_aabb_center(*eaabb);
        float extent = pick_aabb_max_extent(*eaabb);

        // If entity is too large for children, keep it in this node
        if (extent > child_half) {
            pick_octree_link_entry(tree, node_idx, cur);
        } else {
            int octant = pick_octree_octant(tree->nodes[node_idx].center, ecenter);
            pick_octree_link_entry(tree, tree->nodes[node_idx].children[octant], cur);
        }
        cur = next;
    }
}

//------------------------------------------------------------------------------
// Insert
//------------------------------------------------------------------------------

static inline void pick_octree_insert(pick_octree_t *tree, uint64_t entity,
                                       uint32_t pick_id, pick_aabb_t aabb)
{
    if (!tree->initialized) return;

    // Allocate entry
    uint32_t entry_idx = pick_octree_alloc_entry(tree);
    pick_octree_entry_t *entry = &tree->entries[entry_idx];
    entry->entity = entity;
    entry->aabb = aabb;
    entry->node_idx = PICK_OCTREE_NULL;
    entry->next = PICK_OCTREE_NULL;

    // Map pick_id -> entry
    pick_octree_map_ensure(tree, pick_id);
    tree->pick_id_to_entry[pick_id] = entry_idx;

    // Find insertion node by walking from root
    vec3_t ecenter = pick_aabb_center(aabb);
    float extent = pick_aabb_max_extent(aabb);

    uint32_t cur = tree->root;
    while (true) {
        pick_octree_node_t *node = &tree->nodes[cur];

        // If leaf: insert here (possibly split)
        if (node->is_leaf) {
            pick_octree_link_entry(tree, cur, entry_idx);

            // Split if over capacity and not at max depth
            if (node->entry_count > PICK_OCTREE_LEAF_CAPACITY &&
                node->depth < PICK_OCTREE_MAX_DEPTH) {
                pick_octree_split(tree, cur);
            }
            return;
        }

        // Internal node: descend if entity fits in a child
        float child_half = node->half_size * 0.5f;
        if (extent > child_half) {
            // Too large for children — store here
            pick_octree_link_entry(tree, cur, entry_idx);
            return;
        }

        int octant = pick_octree_octant(node->center, ecenter);
        cur = node->children[octant];
    }
}

//------------------------------------------------------------------------------
// Remove
//------------------------------------------------------------------------------

static inline void pick_octree_remove(pick_octree_t *tree, uint32_t pick_id) {
    if (!tree->initialized) return;
    if (pick_id >= tree->pick_id_map_capacity) return;

    uint32_t entry_idx = tree->pick_id_to_entry[pick_id];
    if (entry_idx == PICK_OCTREE_NULL) return;

    uint32_t node_idx = tree->entries[entry_idx].node_idx;
    pick_octree_unlink_entry(tree, node_idx, entry_idx);

    tree->pick_id_to_entry[pick_id] = PICK_OCTREE_NULL;
    pick_octree_free_entry(tree, entry_idx);
}

//------------------------------------------------------------------------------
// Move (remove + reinsert with new AABB)
//------------------------------------------------------------------------------

static inline void pick_octree_move(pick_octree_t *tree, uint64_t entity,
                                     uint32_t pick_id, pick_aabb_t new_aabb)
{
    pick_octree_remove(tree, pick_id);
    pick_octree_insert(tree, entity, pick_id, new_aabb);
}

//------------------------------------------------------------------------------
// Frustum query (iterative stack-based traversal)
//------------------------------------------------------------------------------

static inline void pick_octree_query_frustum(pick_octree_t *tree, const pick_frustum_t *frustum) {
    tree->query_result_count = 0;
    if (!tree->initialized) return;

    uint32_t stack[PICK_OCTREE_QUERY_STACK];
    int stack_top = 0;
    stack[stack_top++] = tree->root;

    while (stack_top > 0) {
        uint32_t ni = stack[--stack_top];
        pick_octree_node_t *node = &tree->nodes[ni];

        // Compute loose AABB (2x the tight half_size)
        float loose = node->half_size * 2.0f;
        pick_aabb_t loose_aabb = {
            .min = vec3_make(node->center.x - loose, node->center.y - loose, node->center.z - loose),
            .max = vec3_make(node->center.x + loose, node->center.y + loose, node->center.z + loose)
        };

        // Test loose AABB against frustum
        if (!pick_aabb_in_frustum(loose_aabb, frustum)) continue;

        // Test each entry in this node
        uint32_t eidx = node->first_entry;
        while (eidx != PICK_OCTREE_NULL) {
            pick_octree_entry_t *e = &tree->entries[eidx];

            if (pick_aabb_in_frustum(e->aabb, frustum)) {
                // Add to results
                if (tree->query_result_count >= tree->query_result_capacity) {
                    tree->query_result_capacity = tree->query_result_capacity * 2;
                    tree->query_results = (uint64_t*)realloc(tree->query_results,
                        tree->query_result_capacity * sizeof(uint64_t));
                }
                tree->query_results[tree->query_result_count++] = e->entity;
            }

            eidx = e->next;
        }

        // Push children onto stack
        if (!node->is_leaf) {
            for (int i = 0; i < 8; i++) {
                if (node->children[i] != PICK_OCTREE_NULL && stack_top < PICK_OCTREE_QUERY_STACK) {
                    stack[stack_top++] = node->children[i];
                }
            }
        }
    }
}

#endif // PICK_OCTREE_H
