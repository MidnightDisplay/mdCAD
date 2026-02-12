# JSONL Mesh Support - Implementation Plan

## Context

The project has an existing JSONL geometry log importer (`jsonl_loader.h`, `jsonl_import_job.h`) that supports wireframe primitives: Point3D, Line3D, Arc3D, PolyLine3D, Polygon3D. Our recent mesh triangles work added support for mesh entities (`GEOM_MESH`) with ECS rendering via `scene_add_mesh()` / `scene_add_mesh_colored()`.

We now need to extend the JSONL loader to parse `MeshBody` elements from .NET geometry logs. This mirrors the PLY mesh import capability added in Sprint 5 of the Mesh Features plan, with the same two import modes:
- **Single Mesh Entity**: Efficient indexed mesh entity (one pick_id for all faces)
- **Individual Triangles**: Each face as a separate selectable entity

## JSONL Mesh Format Analysis

Based on `SomePart.jsonl`, the mesh format is:

```json
{
  "$type": "Geo.NET.Log.GeometryLogEntry, Geo.NET Log",
  "Name": "Part Mesh",
  "Elements": [{
    "$type": "Geo.NET.Log.GeometryLogElement, Geo.NET Log",
    "Name": "Mesh",
    "Element": {
      "$type": "Geo.NET.Shapes.MeshBody, Geo.NET Core",
      "_Points": [
        {"$type": "Geo.NET.Geometry.Point3D, Geo.NET Core", "X": 121.2, "Y": 0.0, "Z": -3.97},
        ...
      ],
      "_Normals": [
        {"$type": "Geo.NET.Geometry.Vector3D, Geo.NET Core", "I": 0.0, "J": 1.0, "K": 0.0},
        ...
      ],
      "_Indices": [
        {"$type": "Geo.NET.Shapes.MeshBody+Index, Geo.NET Core", "PointIndex": 0, "NormalIndex": 0},
        {"$type": "Geo.NET.Shapes.MeshBody+Index, Geo.NET Core", "PointIndex": 1, "NormalIndex": 0},
        {"$type": "Geo.NET.Shapes.MeshBody+Index, Geo.NET Core", "PointIndex": 2, "NormalIndex": 0},
        ...
      ]
    },
    "Colour": "White"
  }]
}
```

**Key characteristics**:
1. **Vertices** (`_Points`): Array of Point3D with X, Y, Z
2. **Normals** (`_Normals`): Separate array of Vector3D with I, J, K (face normals, not per-vertex)
3. **Indices** (`_Indices`): Flat array of `MeshBody+Index` objects, where every 3 consecutive indices form a triangle
4. **Index structure**: Each index has `PointIndex` (vertex ref) and `NormalIndex` (normal ref)
5. **Color**: Single element color (uniform mesh color, not per-vertex)

This is similar to OBJ format with separate position and normal indices, but our ECS mesh system expects per-vertex normals. We'll need to expand face normals to per-vertex during import.

## Scope & Deliverables

### Sprint 1: Parser Extension (jsonl_loader.h)
- [ ] Add `JSONL_GEOM_MESH` enum value
- [ ] Add `jsonl_mesh_data_t` struct for parsed mesh data
- [ ] Add mesh-specific fields to `jsonl_element_t` union
- [ ] Implement `jsonl_parse_mesh_body()` parsing function
- [ ] Integrate mesh parsing into `jsonl_parse_element()`
- [ ] Test: Parse SomePart.jsonl and verify vertex/index counts

### Sprint 2: Entity Creation (jsonl_import_job.h)
- [ ] Add `mesh_import_mode` field to `jsonl_import_job_t` (0 = Single Mesh, 1 = Individual Triangles)
- [ ] Extend `jsonl_import_job_apply_transforms()` for mesh coordinates
- [ ] Add mesh entity creation code in `jsonl_import_job_create_entity()`:
  - Mode 0: Use `scene_add_mesh()` or `scene_add_mesh_colored()`
  - Mode 1: Create individual triangles with `scene_add_triangle()`
- [ ] Handle face-normal-to-vertex-normal expansion (average at shared vertices)
- [ ] Test: Import SomePart.jsonl in both modes, verify rendering

### Sprint 3: UI Integration (ui_scene_hierarchy.h)
- [ ] Add mesh import mode option when JSONL contains mesh elements
- [ ] Show mesh-specific info in options dialog:
  - Mesh vertex count
  - Mesh face (triangle) count
- [ ] Add radio buttons for mesh import mode selection
- [ ] Only show mesh mode options when file contains mesh data
- [ ] Test: Full UI workflow with SomePart.jsonl

---

## Sprint 1: Parser Extension

### 1.1 New Types in jsonl_loader.h

Add after existing `jsonl_geom_type_t` enum:

```c
typedef enum {
    JSONL_GEOM_POINT,
    JSONL_GEOM_LINE,
    JSONL_GEOM_ARC,
    JSONL_GEOM_POLYLINE,
    JSONL_GEOM_POLYGON,
    JSONL_GEOM_MESH,      // NEW
    JSONL_GEOM_UNKNOWN,
} jsonl_geom_type_t;
```

Add mesh data structure:

```c
typedef struct {
    vec3_t *vertices;       // _Points array
    vec3_t *normals;        // _Normals array (face normals)
    int vertex_count;
    int normal_count;
    
    // _Indices array (groups of 3)
    uint32_t *point_indices;   // PointIndex values
    uint32_t *normal_indices;  // NormalIndex values
    int index_count;           // Total indices (must be multiple of 3)
} jsonl_mesh_data_t;
```

Extend `jsonl_element_t` union:

```c
union {
    struct { vec3_t point; } point;
    struct { vec3_t start, end; } line;
    struct { ... } arc;
    struct { vec3_t *points; int count; } polyline;
    struct { jsonl_mesh_data_t mesh; } mesh;  // NEW
} data;
```

### 1.2 Mesh Parsing Function

```c
static inline bool jsonl_parse_mesh_body(const cJSON *json, jsonl_element_t *element) {
    // Get _Points array
    cJSON *points_arr = cJSON_GetObjectItemCaseSensitive(json, "_Points");
    if (!points_arr || !cJSON_IsArray(points_arr)) return false;
    
    int vertex_count = cJSON_GetArraySize(points_arr);
    if (vertex_count < 3) return false;  // Need at least one triangle
    
    // Get _Normals array
    cJSON *normals_arr = cJSON_GetObjectItemCaseSensitive(json, "_Normals");
    if (!normals_arr || !cJSON_IsArray(normals_arr)) return false;
    
    int normal_count = cJSON_GetArraySize(normals_arr);
    
    // Get _Indices array
    cJSON *indices_arr = cJSON_GetObjectItemCaseSensitive(json, "_Indices");
    if (!indices_arr || !cJSON_IsArray(indices_arr)) return false;
    
    int index_count = cJSON_GetArraySize(indices_arr);
    if (index_count < 3 || (index_count % 3) != 0) return false;  // Must be triangle triplets
    
    // Allocate storage
    element->type = JSONL_GEOM_MESH;
    jsonl_mesh_data_t *mesh = &element->data.mesh;
    
    mesh->vertices = (vec3_t*)malloc(sizeof(vec3_t) * vertex_count);
    mesh->normals = (vec3_t*)malloc(sizeof(vec3_t) * normal_count);
    mesh->point_indices = (uint32_t*)malloc(sizeof(uint32_t) * index_count);
    mesh->normal_indices = (uint32_t*)malloc(sizeof(uint32_t) * index_count);
    
    if (!mesh->vertices || !mesh->normals || !mesh->point_indices || !mesh->normal_indices) {
        // Cleanup on failure
        if (mesh->vertices) free(mesh->vertices);
        if (mesh->normals) free(mesh->normals);
        if (mesh->point_indices) free(mesh->point_indices);
        if (mesh->normal_indices) free(mesh->normal_indices);
        return false;
    }
    
    mesh->vertex_count = vertex_count;
    mesh->normal_count = normal_count;
    mesh->index_count = index_count;
    
    // Parse vertices
    int idx = 0;
    cJSON *pt;
    cJSON_ArrayForEach(pt, points_arr) {
        mesh->vertices[idx++] = jsonl_parse_point3d(pt);
    }
    
    // Parse normals (as Vector3D with I, J, K)
    idx = 0;
    cJSON *nm;
    cJSON_ArrayForEach(nm, normals_arr) {
        mesh->normals[idx++] = jsonl_parse_vector3d(nm);
    }
    
    // Parse indices
    idx = 0;
    cJSON *ix;
    cJSON_ArrayForEach(ix, indices_arr) {
        cJSON *pi = cJSON_GetObjectItemCaseSensitive(ix, "PointIndex");
        cJSON *ni = cJSON_GetObjectItemCaseSensitive(ix, "NormalIndex");
        
        mesh->point_indices[idx] = (pi && cJSON_IsNumber(pi)) ? (uint32_t)pi->valuedouble : 0;
        mesh->normal_indices[idx] = (ni && cJSON_IsNumber(ni)) ? (uint32_t)ni->valuedouble : 0;
        idx++;
    }
    
    return true;
}
```

### 1.3 Integration into Parser

In `jsonl_parse_element()`, add case for MeshBody:

```c
else if (strcmp(class_name, "MeshBody") == 0) {
    return jsonl_parse_mesh_body(geom, element);
}
```

### 1.4 Memory Cleanup

Update `jsonl_data_free()` to free mesh data:

```c
if (elem->type == JSONL_GEOM_MESH) {
    if (elem->data.mesh.vertices) free(elem->data.mesh.vertices);
    if (elem->data.mesh.normals) free(elem->data.mesh.normals);
    if (elem->data.mesh.point_indices) free(elem->data.mesh.point_indices);
    if (elem->data.mesh.normal_indices) free(elem->data.mesh.normal_indices);
}
```

### 1.5 Sprint 1 Testing

1. Build and run
2. File > Import JSONL > select SomePart.jsonl
3. Check console/debug output for parsed vertex/index counts
4. Wireframe geometry should still import (second entry in file)

---

## Sprint 2: Entity Creation

### 2.1 Import Mode Field

Add to `jsonl_import_job_t`:

```c
int mesh_import_mode;  // 0 = Single Mesh, 1 = Individual Triangles
```

Initialize in `jsonl_import_job_init()`:

```c
job->mesh_import_mode = 0;  // Default: Single Mesh (efficient)
```

### 2.2 Transform Application

Extend `jsonl_import_job_apply_transforms()` to handle mesh vertices:

```c
case JSONL_GEOM_MESH: {
    jsonl_mesh_data_t *mesh = &elem->data.mesh;
    for (int v = 0; v < mesh->vertex_count; v++) {
        JSONL_TRANSFORM_POINT(mesh->vertices[v]);
    }
    // Rotate normals (but don't translate or scale)
    if (has_rotation) {
        for (int n = 0; n < mesh->normal_count; n++) {
            mesh->normals[n] = mat4_mul_point(rot_matrix, mesh->normals[n]);
            mesh->normals[n] = vec3_normalize(mesh->normals[n]);
        }
    }
    break;
}
```

Also include mesh vertices in CoM calculation:

```c
case JSONL_GEOM_MESH:
    for (int v = 0; v < elem->data.mesh.vertex_count; v++) {
        sum = vec3_add(sum, elem->data.mesh.vertices[v]);
        point_count++;
    }
    break;
```

### 2.3 Face Normal to Vertex Normal Conversion

The JSONL mesh has face normals in `_Normals`, indexed by `NormalIndex`. Our `scene_add_mesh()` expects per-vertex normals. We need to:
1. Create per-vertex normal array (same size as vertices)
2. For each triangle, add face normal to each vertex's accumulator
3. Normalize after accumulation

Helper function (similar to `ply_mesh_face_colors_to_vertex_colors`):

```c
static inline vec3_t* jsonl_face_normals_to_vertex_normals(
    const jsonl_mesh_data_t *mesh) {
    
    vec3_t *vertex_normals = (vec3_t*)calloc(mesh->vertex_count, sizeof(vec3_t));
    if (!vertex_normals) return NULL;
    
    // Accumulate normals at each vertex
    int tri_count = mesh->index_count / 3;
    for (int t = 0; t < tri_count; t++) {
        uint32_t ni = mesh->normal_indices[t * 3];  // Same normal for all 3 vertices
        if ((int)ni >= mesh->normal_count) ni = 0;
        vec3_t fn = mesh->normals[ni];
        
        for (int v = 0; v < 3; v++) {
            uint32_t vi = mesh->point_indices[t * 3 + v];
            if ((int)vi < mesh->vertex_count) {
                vertex_normals[vi] = vec3_add(vertex_normals[vi], fn);
            }
        }
    }
    
    // Normalize
    for (int i = 0; i < mesh->vertex_count; i++) {
        float len = vec3_length(vertex_normals[i]);
        if (len > 0.0001f) {
            vertex_normals[i] = vec3_scale(vertex_normals[i], 1.0f / len);
        } else {
            vertex_normals[i] = vec3_make(0.0f, 1.0f, 0.0f);  // Default up
        }
    }
    
    return vertex_normals;
}
```

### 2.4 Entity Creation

Extend `jsonl_import_job_create_entity()`:

```c
case JSONL_GEOM_MESH: {
    jsonl_mesh_data_t *mesh = &elem->data.mesh;
    int tri_count = mesh->index_count / 3;
    
    if (job->mesh_import_mode == 0) {
        // Mode 0: Single Mesh Entity
        // Build index buffer for scene_add_mesh()
        uint32_t *indices = (uint32_t*)malloc(sizeof(uint32_t) * mesh->index_count);
        memcpy(indices, mesh->point_indices, sizeof(uint32_t) * mesh->index_count);
        
        // Convert face normals to vertex normals
        vec3_t *vertex_normals = jsonl_face_normals_to_vertex_normals(mesh);
        
        // Use scene_add_mesh() - uniform color for now
        ecs_entity_t e = scene_add_mesh(
            scene,
            mesh->vertices, mesh->vertex_count,
            vertex_normals,
            indices, tri_count,
            colour
        );
        
        free(indices);
        free(vertex_normals);
        return e;
    }
    else {
        // Mode 1: Individual Triangles (not handled here - see 2.5)
        // This mode creates multiple entities per element, needs special handling
        return 0;
    }
}
```

### 2.5 Individual Triangles Mode

Individual triangle mode creates multiple entities per mesh element. This requires special handling in the entity creation loop, similar to how PLY mesh import handles it.

In the entity creation state machine, when we encounter a mesh element:

```c
// Special case for mesh in Individual Triangles mode
if (elem->type == JSONL_GEOM_MESH && job->mesh_import_mode == 1) {
    jsonl_mesh_data_t *mesh = &elem->data.mesh;
    int tri_count = mesh->index_count / 3;
    
    // Create triangles in chunks
    while (job->mesh_tri_created < tri_count && created_this_frame < JSONL_ENTITY_CHUNK_SIZE) {
        int t = job->mesh_tri_created;
        
        uint32_t i0 = mesh->point_indices[t * 3 + 0];
        uint32_t i1 = mesh->point_indices[t * 3 + 1];
        uint32_t i2 = mesh->point_indices[t * 3 + 2];
        
        vec3_t a = mesh->vertices[i0];
        vec3_t b = mesh->vertices[i1];
        vec3_t c = mesh->vertices[i2];
        
        ecs_entity_t tri = scene_add_triangle(scene, a, b, c, colour);
        // Parent to entry anchor...
        
        job->mesh_tri_created++;
        created_this_frame++;
        job->total_entities_created++;
    }
}
```

### 2.6 Sprint 2 Testing

1. Import SomePart.jsonl in Single Mesh mode
   - Verify mesh renders with correct geometry
   - Verify clicking selects entire mesh
   - Verify lighting works (normals correct)

2. Import SomePart.jsonl in Individual Triangles mode
   - Verify all triangles render
   - Verify clicking individual triangles selects them
   - Note: May be slow for large meshes, that's expected

---

## Sprint 3: UI Integration

### 3.1 Mesh Detection

Add helper to check if parsed JSONL contains mesh elements:

```c
static inline bool jsonl_data_has_mesh(const jsonl_data_t *data) {
    for (int e = 0; e < data->entry_count; e++) {
        for (int i = 0; i < data->entries[e].element_count; i++) {
            if (data->entries[e].elements[i].type == JSONL_GEOM_MESH) {
                return true;
            }
        }
    }
    return false;
}

static inline int jsonl_data_mesh_vertex_count(const jsonl_data_t *data) {
    int total = 0;
    for (int e = 0; e < data->entry_count; e++) {
        for (int i = 0; i < data->entries[e].element_count; i++) {
            if (data->entries[e].elements[i].type == JSONL_GEOM_MESH) {
                total += data->entries[e].elements[i].data.mesh.vertex_count;
            }
        }
    }
    return total;
}

static inline int jsonl_data_mesh_face_count(const jsonl_data_t *data) {
    int total = 0;
    for (int e = 0; e < data->entry_count; e++) {
        for (int i = 0; i < data->entries[e].element_count; i++) {
            if (data->entries[e].elements[i].type == JSONL_GEOM_MESH) {
                total += data->entries[e].elements[i].data.mesh.index_count / 3;
            }
        }
    }
    return total;
}
```

### 3.2 UI State Fields

Add to `ui_scene_hierarchy_state_t`:

```c
int jsonl_mesh_import_mode;  // 0 = Single Mesh, 1 = Individual Triangles
bool jsonl_has_mesh_data;    // Set after quick scan
int jsonl_mesh_vertex_count;
int jsonl_mesh_face_count;
```

### 3.3 Import Options Dialog Update

In the JSONL import options popup, add mesh mode selection when mesh data is present:

```c
if (state->jsonl_has_mesh_data) {
    igSeparator();
    igText("Mesh Import Options");
    igText("Mesh vertices: %d, faces: %d", 
           state->jsonl_mesh_vertex_count, state->jsonl_mesh_face_count);
    
    igRadioButton_IntPtr("Single Mesh Entity (efficient)", 
                         &state->jsonl_mesh_import_mode, 0);
    if (igIsItemHovered(0)) {
        igSetTooltip("Import mesh as one indexed entity (recommended)");
    }
    
    igRadioButton_IntPtr("Individual Triangles (selectable)", 
                         &state->jsonl_mesh_import_mode, 1);
    if (igIsItemHovered(0)) {
        igSetTooltip("Each triangle as separate entity - useful for editing");
    }
}
```

### 3.4 Pass Mode to Import Job

When starting the import:

```c
jsonl_import_job_start_ex(&state->jsonl_import_job,
    state->jsonl_import_path,
    scale, use_colours, default_colour,
    shift_to_com, rotation_x, rotation_y, rotation_z,
    state->jsonl_mesh_import_mode  // NEW
);
```

Or add a setter:

```c
jsonl_import_job_set_mesh_mode(&state->jsonl_import_job, state->jsonl_mesh_import_mode);
```

### 3.5 Sprint 3 Testing

Full end-to-end test:
1. File > Import JSONL > SomePart.jsonl
2. Verify options dialog shows mesh info
3. Test Single Mesh mode - efficient, single selection
4. Test Individual Triangles mode - many entities, individual selection
5. Test with transform options (scale, rotation, CoM shift)
6. Test cancel during import

---

## Implementation Checklist

### Sprint 1: Parser Extension
- [ ] Add `JSONL_GEOM_MESH` to `jsonl_geom_type_t` enum
- [ ] Add `jsonl_mesh_data_t` struct
- [ ] Extend `jsonl_element_t` union with mesh field
- [ ] Implement `jsonl_parse_mesh_body()` function
- [ ] Add mesh case to `jsonl_parse_element()`
- [ ] Update `jsonl_data_free()` for mesh memory cleanup
- [ ] Verify SomePart.jsonl parses without errors

### Sprint 2: Entity Creation
- [ ] Add `mesh_import_mode` to `jsonl_import_job_t`
- [ ] Extend transform application for mesh vertices/normals
- [ ] Implement `jsonl_face_normals_to_vertex_normals()`
- [ ] Add Single Mesh mode entity creation
- [ ] Add Individual Triangles mode entity creation (chunked)
- [ ] Test both modes with SomePart.jsonl

### Sprint 3: UI Integration
- [ ] Add mesh detection helpers
- [ ] Add UI state fields for mesh options
- [ ] Extend quick scan to detect mesh data
- [ ] Add mesh import mode radio buttons to dialog
- [ ] Pass mesh mode to import job
- [ ] Full workflow test

---

## Files Modified

| File | Changes |
|------|---------|
| `src/jsonl_loader.h` | Add mesh type, struct, parser |
| `src/jsonl_import_job.h` | Mesh mode, transform, entity creation |
| `src/ui/ui_scene_hierarchy.h` | Mesh UI options |

---

## Future Considerations

1. **Per-vertex colors from JSONL**: If future JSONL exports include vertex colors, extend parser
2. **Multiple meshes per element**: Current format uses one MeshBody per element; if multiple, handle as separate entities
3. **Mesh wire overlay**: Option to show mesh edges alongside filled triangles
4. **Large mesh chunking**: For very large meshes (>100k triangles), consider chunked parsing similar to PLY binary
