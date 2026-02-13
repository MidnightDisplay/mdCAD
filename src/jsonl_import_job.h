//------------------------------------------------------------------------------
// jsonl_import_job.h - JSONL import job state machine (header-only)
//
// Provides chunked JSONL import with progress tracking:
// - Parses JSONL file in chunks across frames
// - Creates ECS geometry entities in chunks
// - Sets parent-child relationships in chunks
// - Progress callback for UI updates
// - Cancellation support
// - Import transformations: CoM shift, rotation, scale
//------------------------------------------------------------------------------
#ifndef JSONL_IMPORT_JOB_H
#define JSONL_IMPORT_JOB_H

#include "jsonl_loader.h"
#include "ecs/ecs_scene.h"
#include "sokol_time.h"
#include <string.h>

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------

#define JSONL_PARSE_CHUNK_SIZE      50      // Lines per frame during parsing
#define JSONL_ENTITY_CHUNK_SIZE     200     // Entities per frame during creation
#define JSONL_PARENT_CHUNK_SIZE     500     // Parenting ops per frame
#define JSONL_SYNC_THRESHOLD        100     // Total elements below this import synchronously
#define JSONL_DEFAULT_LINE_WIDTH    0.03f   // Default line width for imported geometry

//------------------------------------------------------------------------------
// Job states
//------------------------------------------------------------------------------

typedef enum {
    JSONL_JOB_IDLE = 0,
    JSONL_JOB_PARSING_FILE,         // Reading and parsing JSONL
    JSONL_JOB_CREATING_ENTITIES,    // Creating ECS geometry entities
    JSONL_JOB_PARENTING_ENTITIES,   // Setting parent-child relationships
    JSONL_JOB_COMPLETE,
    JSONL_JOB_CANCELLED,
    JSONL_JOB_ERROR
} jsonl_job_state_t;

//------------------------------------------------------------------------------
// Import job structure
//------------------------------------------------------------------------------

#define JSONL_JOB_MAX_TIMING_SAMPLES 100

typedef struct {
    jsonl_job_state_t state;
    char filepath[512];

    // Import options
    float scale;
    bool use_jsonl_colours;
    vec4_t default_colour;
    bool shift_to_com;
    float rotation_x, rotation_y, rotation_z;  // radians
    int mesh_import_mode;  // 0 = Single Mesh, 1 = Individual Triangles

    // Parse state
    jsonl_parse_state_t parse_state;

    // Entity creation state
    int current_entry_idx;      // which log entry we're processing
    int current_element_idx;    // which element within that entry
    int total_entities_created;
    ecs_entity_t root_entity;
    ecs_entity_t *entry_entities;  // one per log entry (sub-anchors)

    // Parenting state
    int parented_count;
    int total_to_parent;
    ecs_entity_t *all_created_entities;   // flat array of all geometry entities
    int *entity_to_entry_map;             // maps each entity index to its entry index
    int all_created_count;

    // Mesh triangles mode state (for Individual Triangles import)
    int mesh_tri_created;       // triangles created so far for current mesh
    int mesh_tri_total;         // total triangles in current mesh

    // Computed transforms
    vec3_t com;
    mat4_t transform_matrix;
    bool transforms_applied;

    // Progress & timing
    float progress;
    char status_message[128];
    double last_iteration_time_ms;
    float iteration_times[JSONL_JOB_MAX_TIMING_SAMPLES];
    float iteration_progress[JSONL_JOB_MAX_TIMING_SAMPLES];
    int timing_sample_count;
    float last_sampled_progress;

    // Result
    jsonl_error_t error;
    int total_elements;
} jsonl_import_job_t;

//------------------------------------------------------------------------------
// Initialize job structure
//------------------------------------------------------------------------------

static inline void jsonl_import_job_init(jsonl_import_job_t *job) {
    memset(job, 0, sizeof(jsonl_import_job_t));
    job->state = JSONL_JOB_IDLE;
    job->scale = 1.0f;
    job->use_jsonl_colours = true;
    job->default_colour = vec4_make(1.0f, 1.0f, 1.0f, 1.0f);
    job->shift_to_com = false;
    job->rotation_x = 0.0f;
    job->rotation_y = 0.0f;
    job->rotation_z = 0.0f;
    job->mesh_import_mode = 0;  // Default: Single Mesh (efficient)
    job->transforms_applied = false;
    job->last_iteration_time_ms = 0.0;
    job->timing_sample_count = 0;
    job->last_sampled_progress = -1.0f;
    job->mesh_tri_created = 0;
    job->mesh_tri_total = 0;
}

//------------------------------------------------------------------------------
// Start a new import job
//------------------------------------------------------------------------------

static inline bool jsonl_import_job_start(jsonl_import_job_t *job,
                                           const char *filepath,
                                           float scale,
                                           bool use_jsonl_colours,
                                           vec4_t default_colour,
                                           bool shift_to_com,
                                           float rotation_x,
                                           float rotation_y,
                                           float rotation_z) {
    // Cancel any running job first
    if (job->state != JSONL_JOB_IDLE &&
        job->state != JSONL_JOB_COMPLETE &&
        job->state != JSONL_JOB_CANCELLED &&
        job->state != JSONL_JOB_ERROR) {
        jsonl_parse_state_free(&job->parse_state);
    }

    // Store options
    strncpy(job->filepath, filepath, sizeof(job->filepath) - 1);
    job->filepath[sizeof(job->filepath) - 1] = '\0';
    job->scale = scale;
    job->use_jsonl_colours = use_jsonl_colours;
    job->default_colour = default_colour;
    job->shift_to_com = shift_to_com;
    job->rotation_x = rotation_x;
    job->rotation_y = rotation_y;
    job->rotation_z = rotation_z;

    // Reset state
    job->current_entry_idx = 0;
    job->current_element_idx = 0;
    job->total_entities_created = 0;
    job->root_entity = 0;
    job->entry_entities = NULL;
    job->parented_count = 0;
    job->total_to_parent = 0;
    job->all_created_entities = NULL;
    job->entity_to_entry_map = NULL;
    job->all_created_count = 0;
    job->mesh_tri_created = 0;
    job->mesh_tri_total = 0;
    job->progress = 0.0f;
    job->error = JSONL_OK;
    job->total_elements = 0;
    job->transforms_applied = false;
    job->com = vec3_make(0, 0, 0);
    job->transform_matrix = mat4_identity();

    // Reset timing data
    job->last_iteration_time_ms = 0.0;
    job->timing_sample_count = 0;
    job->last_sampled_progress = -1.0f;

    // Open file and count lines
    jsonl_error_t err = jsonl_open(filepath, &job->parse_state);
    if (err != JSONL_OK) {
        job->state = JSONL_JOB_ERROR;
        job->error = err;
        snprintf(job->status_message, sizeof(job->status_message),
                 "Error: %s", jsonl_error_string(err));
        return false;
    }

    job->state = JSONL_JOB_PARSING_FILE;
    snprintf(job->status_message, sizeof(job->status_message),
             "Parsing: 0 / %d lines", job->parse_state.total_lines);

    // Record initial 0% sample for the plot
    job->iteration_times[0] = 0.0f;
    job->iteration_progress[0] = 0.0f;
    job->timing_sample_count = 1;

    return true;
}

//------------------------------------------------------------------------------
// Check if job should use synchronous import (small file)
//------------------------------------------------------------------------------

static inline bool jsonl_import_job_should_sync(const jsonl_import_job_t *job) {
    // Use total_lines as a proxy before we know total_elements
    return job->parse_state.total_lines < 5;
}

//------------------------------------------------------------------------------
// Internal: Apply transforms to all parsed geometry coordinates
// Order: 1) CoM shift, 2) Rotation (X->Y->Z), 3) Scale
//------------------------------------------------------------------------------

static inline void jsonl_import_job_apply_transforms(jsonl_import_job_t *job) {
    if (job->transforms_applied) return;

    jsonl_data_t *data = &job->parse_state.data;

    // Step 1: Collect all characteristic points for CoM calculation
    if (job->shift_to_com) {
        vec3_t sum = vec3_make(0, 0, 0);
        int point_count = 0;

        for (int e = 0; e < data->entry_count; e++) {
            jsonl_log_entry_t *entry = &data->entries[e];
            for (int i = 0; i < entry->element_count; i++) {
                jsonl_element_t *elem = &entry->elements[i];
                switch (elem->type) {
                    case JSONL_GEOM_POINT:
                        sum = vec3_add(sum, elem->data.point.point);
                        point_count++;
                        break;
                    case JSONL_GEOM_LINE:
                        sum = vec3_add(sum, elem->data.line.start);
                        sum = vec3_add(sum, elem->data.line.end);
                        point_count += 2;
                        break;
                    case JSONL_GEOM_ARC:
                        sum = vec3_add(sum, elem->data.arc.center);
                        point_count++;
                        break;
                    case JSONL_GEOM_POLYLINE:
                    case JSONL_GEOM_POLYGON:
                        for (int p = 0; p < elem->data.polyline.count; p++) {
                            sum = vec3_add(sum, elem->data.polyline.points[p]);
                            point_count++;
                        }
                        break;
                    case JSONL_GEOM_MESH: {
                        jsonl_mesh_data_t *mesh = &elem->data.mesh.mesh;
                        for (int v = 0; v < mesh->vertex_count; v++) {
                            sum = vec3_add(sum, mesh->vertices[v]);
                            point_count++;
                        }
                        break;
                    }
                    default: break;
                }
            }
        }

        if (point_count > 0) {
            job->com = vec3_scale(sum, 1.0f / (float)point_count);
        }
    }

    // Step 2: Build rotation matrix
    bool has_rotation = (job->rotation_x != 0.0f ||
                         job->rotation_y != 0.0f ||
                         job->rotation_z != 0.0f);

    mat4_t rot_matrix = mat4_identity();
    if (has_rotation) {
        mat4_t rot_x = mat4_rotate_x(job->rotation_x);
        mat4_t rot_y = mat4_rotate_y(job->rotation_y);
        mat4_t rot_z = mat4_rotate_z(job->rotation_z);
        mat4_t rot_xy = mat4_mul(rot_y, rot_x);
        rot_matrix = mat4_mul(rot_z, rot_xy);
        job->transform_matrix = rot_matrix;
    }

    // Apply transforms to all geometry coordinates
    for (int e = 0; e < data->entry_count; e++) {
        jsonl_log_entry_t *entry = &data->entries[e];
        for (int i = 0; i < entry->element_count; i++) {
            jsonl_element_t *elem = &entry->elements[i];

            // Helper lambda-like: transform a single point
            #define JSONL_TRANSFORM_POINT(pt) do { \
                if (job->shift_to_com) pt = vec3_sub(pt, job->com); \
                if (has_rotation) pt = mat4_mul_point(rot_matrix, pt); \
                if (job->scale != 1.0f) pt = vec3_scale(pt, job->scale); \
            } while(0)

            switch (elem->type) {
                case JSONL_GEOM_POINT:
                    JSONL_TRANSFORM_POINT(elem->data.point.point);
                    break;
                case JSONL_GEOM_LINE:
                    JSONL_TRANSFORM_POINT(elem->data.line.start);
                    JSONL_TRANSFORM_POINT(elem->data.line.end);
                    break;
                case JSONL_GEOM_ARC: {
                    JSONL_TRANSFORM_POINT(elem->data.arc.center);
                    if (job->scale != 1.0f) {
                        elem->data.arc.radius *= job->scale;
                    }
                    if (has_rotation) {
                        elem->data.arc.normal = mat4_mul_point(rot_matrix, elem->data.arc.normal);
                        elem->data.arc.normal = vec3_normalize(elem->data.arc.normal);
                    }
                    // Note: angles stay the same - they're relative to the local coordinate
                    // system which rotates with the normal
                    break;
                }
                case JSONL_GEOM_POLYLINE:
                case JSONL_GEOM_POLYGON:
                    for (int p = 0; p < elem->data.polyline.count; p++) {
                        JSONL_TRANSFORM_POINT(elem->data.polyline.points[p]);
                    }
                    break;
                case JSONL_GEOM_MESH: {
                    jsonl_mesh_data_t *mesh = &elem->data.mesh.mesh;
                    // Transform vertices
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
                default: break;
            }

            #undef JSONL_TRANSFORM_POINT
        }
    }

    job->transforms_applied = true;
}

//------------------------------------------------------------------------------
// Internal: Create one geometry entity from a parsed element
//------------------------------------------------------------------------------

static inline ecs_entity_t jsonl_import_job_create_entity(
    jsonl_import_job_t *job,
    ecs_scene_t *scene,
    jsonl_element_t *elem)
{
    vec4_t colour = job->use_jsonl_colours ? elem->colour : job->default_colour;
    float width = JSONL_DEFAULT_LINE_WIDTH;

    switch (elem->type) {
        case JSONL_GEOM_POINT:
            return scene_add_point(scene, elem->data.point.point, colour, 0.06f);

        case JSONL_GEOM_LINE:
            return scene_add_line(scene, elem->data.line.start, elem->data.line.end,
                                  colour, width);

        case JSONL_GEOM_ARC:
            return scene_add_arc(scene,
                elem->data.arc.center,
                elem->data.arc.radius,
                elem->data.arc.start_angle,
                elem->data.arc.end_angle,
                elem->data.arc.normal,
                colour, width);

        case JSONL_GEOM_POLYLINE:
            return scene_add_polyline(scene, elem->data.polyline.points,
                                      elem->data.polyline.count, colour, width);

        case JSONL_GEOM_POLYGON:
            return scene_add_polygon(scene, elem->data.polyline.points,
                                     elem->data.polyline.count, colour, width);

        case JSONL_GEOM_MESH: {
            // Mesh handled separately - needs special logic for import modes
            // This case only handles Single Mesh mode (mode 0);
            // Individual Triangles mode (mode 1) is handled in tick()
            if (job->mesh_import_mode == 0) {
                jsonl_mesh_data_t *mesh = &elem->data.mesh.mesh;
                int index_count = mesh->index_count;
                return scene_add_mesh(scene,
                                      mesh->vertices, mesh->vertex_count,
                                      mesh->point_indices, index_count,
                                      colour);
            }
            return 0;  // Individual Triangles mode handled elsewhere
        }

        default:
            return 0;
    }
}

//------------------------------------------------------------------------------
// Process one chunk of work - returns true when job is complete
//------------------------------------------------------------------------------

static inline bool jsonl_import_job_tick(jsonl_import_job_t *job, ecs_scene_t *scene) {
    if (job->state == JSONL_JOB_IDLE ||
        job->state == JSONL_JOB_COMPLETE ||
        job->state == JSONL_JOB_CANCELLED ||
        job->state == JSONL_JOB_ERROR) {
        return true;
    }

    //--------------------------------------------------------------------------
    // State: PARSING_FILE (0-20% progress)
    //--------------------------------------------------------------------------
    if (job->state == JSONL_JOB_PARSING_FILE) {
        uint64_t iter_start = stm_now();

        int parsed = jsonl_parse_lines_chunk(&job->parse_state, JSONL_PARSE_CHUNK_SIZE);
        (void)parsed;

        uint64_t iter_end = stm_now();
        job->last_iteration_time_ms = stm_ms(stm_diff(iter_end, iter_start));

        if (job->parse_state.error != JSONL_OK) {
            job->state = JSONL_JOB_ERROR;
            job->error = job->parse_state.error;
            snprintf(job->status_message, sizeof(job->status_message),
                     "Parse error: %s", jsonl_error_string(job->error));
            jsonl_parse_state_free(&job->parse_state);
            return true;
        }

        // Parsing is 0-20% of total progress
        job->progress = 0.20f * jsonl_get_progress(&job->parse_state);

        // Store timing sample
        float progress_pct = job->progress * 100.0f;
        if (job->timing_sample_count < JSONL_JOB_MAX_TIMING_SAMPLES &&
            (progress_pct - job->last_sampled_progress) >= 1.0f) {
            job->iteration_times[job->timing_sample_count] = (float)job->last_iteration_time_ms;
            job->iteration_progress[job->timing_sample_count] = progress_pct;
            job->timing_sample_count++;
            job->last_sampled_progress = progress_pct;
        }

        snprintf(job->status_message, sizeof(job->status_message),
                 "Parsing: %d / %d lines",
                 job->parse_state.lines_parsed, job->parse_state.total_lines);

        // Check if parsing is complete
        if (jsonl_is_complete(&job->parse_state)) {
            jsonl_close(&job->parse_state);  // Close file, keep data

            job->total_elements = job->parse_state.data.total_elements;

            // Apply all transformations to parsed data
            jsonl_import_job_apply_transforms(job);

            // Compute entity count for allocation
            // In Individual Triangles mode, meshes create multiple entities
            int expected_entities = job->total_elements;
            if (job->mesh_import_mode == 1) {
                // Add extra space for mesh triangles (each mesh element -> N triangles)
                jsonl_data_t *data = &job->parse_state.data;
                for (int e = 0; e < data->entry_count; e++) {
                    for (int i = 0; i < data->entries[e].element_count; i++) {
                        jsonl_element_t *elem = &data->entries[e].elements[i];
                        if (elem->type == JSONL_GEOM_MESH) {
                            int tri_count = elem->data.mesh.mesh.index_count / 3;
                            expected_entities += (tri_count - 1);  // -1 because we already counted 1
                        }
                    }
                }
            }

            // Allocate tracking arrays
            int num_entries = job->parse_state.data.entry_count;
            job->entry_entities = (ecs_entity_t*)calloc(num_entries, sizeof(ecs_entity_t));
            job->all_created_entities = (ecs_entity_t*)malloc(
                sizeof(ecs_entity_t) * (expected_entities + num_entries + 1));
            job->entity_to_entry_map = (int*)malloc(
                sizeof(int) * (expected_entities + num_entries + 1));

            if (!job->entry_entities || !job->all_created_entities || !job->entity_to_entry_map) {
                job->state = JSONL_JOB_ERROR;
                job->error = JSONL_ERROR_MEMORY_ALLOCATION;
                snprintf(job->status_message, sizeof(job->status_message),
                         "Memory allocation failed");
                return true;
            }

            // Create root anchor entity (transform-only, no GPU slot)
            {
                const char *fname = job->filepath;
                const char *sep = strrchr(job->filepath, '/');
#ifdef _WIN32
                const char *sep_win = strrchr(job->filepath, '\\');
                if (sep_win > sep) sep = sep_win;
#endif
                if (sep) fname = sep + 1;
                job->root_entity = scene_add_anchor(scene, fname, "JSONL import root");
            }

            // Create entry anchor entities (one per log entry, transform-only)
            for (int i = 0; i < num_entries; i++) {
                job->entry_entities[i] = scene_add_anchor(scene,
                    job->parse_state.data.entries[i].name,
                    job->parse_state.data.entries[i].description);
            }

            job->current_entry_idx = 0;
            job->current_element_idx = 0;
            job->all_created_count = 0;
            job->state = JSONL_JOB_CREATING_ENTITIES;
            snprintf(job->status_message, sizeof(job->status_message),
                     "Creating: 0 / %d entities", job->total_elements);
        }

        return false;
    }

    //--------------------------------------------------------------------------
    // State: CREATING_ENTITIES (20-80% progress)
    //--------------------------------------------------------------------------
    if (job->state == JSONL_JOB_CREATING_ENTITIES) {
        uint64_t iter_start = stm_now();

        jsonl_data_t *data = &job->parse_state.data;
        int created_this_frame = 0;

        while (created_this_frame < JSONL_ENTITY_CHUNK_SIZE &&
               job->current_entry_idx < data->entry_count) {

            jsonl_log_entry_t *entry = &data->entries[job->current_entry_idx];

            while (job->current_element_idx < entry->element_count &&
                   created_this_frame < JSONL_ENTITY_CHUNK_SIZE) {

                jsonl_element_t *elem = &entry->elements[job->current_element_idx];

                // Special handling for mesh in Individual Triangles mode
                if (elem->type == JSONL_GEOM_MESH && job->mesh_import_mode == 1) {
                    jsonl_mesh_data_t *mesh = &elem->data.mesh.mesh;
                    int tri_count = mesh->index_count / 3;

                    // Initialize mesh triangle state if starting new mesh
                    if (job->mesh_tri_created == 0) {
                        job->mesh_tri_total = tri_count;
                    }

                    vec4_t colour = job->use_jsonl_colours ? elem->colour : job->default_colour;

                    // Create triangles in chunks
                    while (job->mesh_tri_created < tri_count &&
                           created_this_frame < JSONL_ENTITY_CHUNK_SIZE) {
                        int t = job->mesh_tri_created;

                        uint32_t i0 = mesh->point_indices[t * 3 + 0];
                        uint32_t i1 = mesh->point_indices[t * 3 + 1];
                        uint32_t i2 = mesh->point_indices[t * 3 + 2];

                        // Bounds check
                        if ((int)i0 < mesh->vertex_count &&
                            (int)i1 < mesh->vertex_count &&
                            (int)i2 < mesh->vertex_count) {

                            vec3_t a = mesh->vertices[i0];
                            vec3_t b = mesh->vertices[i1];
                            vec3_t c = mesh->vertices[i2];

                            ecs_entity_t tri = scene_add_triangle(scene, a, b, c, colour);
                            if (tri != 0) {
                                // Tag as import-pending
                                ecs_add_id(scene->world->world, tri, scene->world->ImportPending_tag);
                                // Track for parenting
                                job->all_created_entities[job->all_created_count] = tri;
                                job->entity_to_entry_map[job->all_created_count] = job->current_entry_idx;
                                job->all_created_count++;
                                job->total_entities_created++;
                            }
                        }

                        job->mesh_tri_created++;
                        created_this_frame++;
                    }

                    // Finished this mesh?
                    if (job->mesh_tri_created >= tri_count) {
                        job->mesh_tri_created = 0;
                        job->mesh_tri_total = 0;
                        job->current_element_idx++;
                    }
                }
                else {
                    // Normal element handling (non-mesh or Single Mesh mode)
                    ecs_entity_t e = jsonl_import_job_create_entity(job, scene, elem);

                    if (e != 0) {
                        // Tag as import-pending
                        ecs_add_id(scene->world->world, e, scene->world->ImportPending_tag);

                        // Set label if element has a name
                        if (elem->name[0] || elem->description[0]) {
                            LabelComp lbl = label_comp_make(elem->name, elem->description);
                            ecs_world_set_label(scene->world, e, &lbl);
                        }

                        // Track for parenting
                        job->all_created_entities[job->all_created_count] = e;
                        job->entity_to_entry_map[job->all_created_count] = job->current_entry_idx;
                        job->all_created_count++;
                        job->total_entities_created++;
                    }

                    job->current_element_idx++;
                    created_this_frame++;
                }
            }

            // Move to next entry if current one is done
            if (job->current_element_idx >= entry->element_count) {
                job->current_entry_idx++;
                job->current_element_idx = 0;
            }
        }

        uint64_t iter_end = stm_now();
        job->last_iteration_time_ms = stm_ms(stm_diff(iter_end, iter_start));

        // Update progress (20-80%)
        float entity_progress = (job->total_elements > 0) ?
            (float)job->total_entities_created / (float)job->total_elements : 1.0f;
        job->progress = 0.20f + 0.60f * entity_progress;

        // Store timing sample
        float progress_pct = job->progress * 100.0f;
        if (job->timing_sample_count < JSONL_JOB_MAX_TIMING_SAMPLES &&
            (progress_pct - job->last_sampled_progress) >= 1.0f) {
            job->iteration_times[job->timing_sample_count] = (float)job->last_iteration_time_ms;
            job->iteration_progress[job->timing_sample_count] = progress_pct;
            job->timing_sample_count++;
            job->last_sampled_progress = progress_pct;
        }

        snprintf(job->status_message, sizeof(job->status_message),
                 "Creating: %d / %d entities",
                 job->total_entities_created, job->total_elements);

        // Check if entity creation is complete
        if (job->current_entry_idx >= data->entry_count) {
            // Set up parenting phase
            // Total to parent: all_created_count (geometry -> entry) + entry_count (entry -> root)
            job->total_to_parent = job->all_created_count + data->entry_count;
            job->parented_count = 0;
            job->state = JSONL_JOB_PARENTING_ENTITIES;
            snprintf(job->status_message, sizeof(job->status_message),
                     "Parenting: 0 / %d", job->total_to_parent);
        }

        return false;
    }

    //--------------------------------------------------------------------------
    // State: PARENTING_ENTITIES (80-100% progress)
    // Batch parent all entities in one deferred step
    //--------------------------------------------------------------------------
    if (job->state == JSONL_JOB_PARENTING_ENTITIES) {
        jsonl_data_t *data = &job->parse_state.data;
        ecs_world_state_t *w = scene->world;

        snprintf(job->status_message, sizeof(job->status_message),
                 "Parenting %d entities...", job->all_created_count + data->entry_count);

        // Batch parent + remove ImportPending in one defer block
        ecs_defer_begin(w->world);
        // Parent geometry entities to entry anchors
        for (int i = 0; i < job->all_created_count; i++) {
            ecs_entity_t child = job->all_created_entities[i];
            int entry_idx = job->entity_to_entry_map[i];
            ecs_entity_t parent = job->entry_entities[entry_idx];
            if (child && parent) {
                ecs_add_pair(w->world, child, EcsChildOf, parent);
                ecs_remove_id(w->world, child, w->ImportPending_tag);
            }
        }
        // Parent entry anchors to root
        for (int i = 0; i < data->entry_count; i++) {
            if (job->entry_entities[i] && job->root_entity) {
                ecs_add_pair(w->world, job->entry_entities[i], EcsChildOf, job->root_entity);
            }
        }
        ecs_defer_end(w->world);

        // Mark all dirty (after flush)
        for (int i = 0; i < job->all_created_count; i++) {
            TransformComp *t = ecs_world_get_transform(w, job->all_created_entities[i]);
            if (t) t->dirty = true;
            RenderableComp *r = ecs_world_get_renderable(w, job->all_created_entities[i]);
            if (r) r->instance_dirty = true;
        }

        // Store entry count before freeing
        int num_entries = data->entry_count;

        // Free tracking arrays
        if (job->all_created_entities) {
            free(job->all_created_entities);
            job->all_created_entities = NULL;
        }
        if (job->entity_to_entry_map) {
            free(job->entity_to_entry_map);
            job->entity_to_entry_map = NULL;
        }
        if (job->entry_entities) {
            free(job->entry_entities);
            job->entry_entities = NULL;
        }

        // Free parsed data
        jsonl_parse_state_free(&job->parse_state);

        job->progress = 1.0f;
        job->state = JSONL_JOB_COMPLETE;

        // Record final 100% sample
        if (job->timing_sample_count < JSONL_JOB_MAX_TIMING_SAMPLES) {
            job->iteration_times[job->timing_sample_count] = (float)job->last_iteration_time_ms;
            job->iteration_progress[job->timing_sample_count] = 100.0f;
            job->timing_sample_count++;
        }

        snprintf(job->status_message, sizeof(job->status_message),
                 "Imported %d elements in %d entries",
                 job->total_elements, num_entries);
        return true;
    }

    return true;  // Unknown state
}

//------------------------------------------------------------------------------
// Cancel an in-progress job
//------------------------------------------------------------------------------

static inline void jsonl_import_job_cancel(jsonl_import_job_t *job, ecs_scene_t *scene) {
    if (job->state == JSONL_JOB_IDLE ||
        job->state == JSONL_JOB_COMPLETE ||
        job->state == JSONL_JOB_CANCELLED ||
        job->state == JSONL_JOB_ERROR) {
        return;
    }

    // Remove ImportPending tags from any entities created so far
    if (job->all_created_entities && scene) {
        ecs_world_state_t *w = scene->world;
        for (int i = 0; i < job->all_created_count; i++) {
            if (ecs_is_alive(w->world, job->all_created_entities[i])) {
                ecs_remove_id(w->world, job->all_created_entities[i], w->ImportPending_tag);
            }
        }
    }

    // Free parsed data
    jsonl_parse_state_free(&job->parse_state);

    // Free tracking arrays
    if (job->all_created_entities) {
        free(job->all_created_entities);
        job->all_created_entities = NULL;
    }
    if (job->entity_to_entry_map) {
        free(job->entity_to_entry_map);
        job->entity_to_entry_map = NULL;
    }
    if (job->entry_entities) {
        free(job->entry_entities);
        job->entry_entities = NULL;
    }

    job->state = JSONL_JOB_CANCELLED;
    job->progress = 0.0f;
    snprintf(job->status_message, sizeof(job->status_message), "Import cancelled");
}

//------------------------------------------------------------------------------
// Check if job is currently running
//------------------------------------------------------------------------------

static inline bool jsonl_import_job_is_running(const jsonl_import_job_t *job) {
    return job->state == JSONL_JOB_PARSING_FILE ||
           job->state == JSONL_JOB_CREATING_ENTITIES ||
           job->state == JSONL_JOB_PARENTING_ENTITIES;
}

//------------------------------------------------------------------------------
// Reset job to idle state
//------------------------------------------------------------------------------

static inline void jsonl_import_job_reset(jsonl_import_job_t *job) {
    if (jsonl_import_job_is_running(job)) {
        jsonl_parse_state_free(&job->parse_state);
        if (job->all_created_entities) {
            free(job->all_created_entities);
            job->all_created_entities = NULL;
        }
        if (job->entity_to_entry_map) {
            free(job->entity_to_entry_map);
            job->entity_to_entry_map = NULL;
        }
        if (job->entry_entities) {
            free(job->entry_entities);
            job->entry_entities = NULL;
        }
    }
    job->state = JSONL_JOB_IDLE;
    job->progress = 0.0f;
    job->status_message[0] = '\0';
    job->transforms_applied = false;
}

//------------------------------------------------------------------------------
// Set mesh import mode (call before starting job)
// 0 = Single Mesh Entity (efficient, one pick_id for entire mesh)
// 1 = Individual Triangles (each face is a separate selectable entity)
//------------------------------------------------------------------------------

static inline void jsonl_import_job_set_mesh_mode(jsonl_import_job_t *job, int mode) {
    job->mesh_import_mode = mode;
}

#endif // JSONL_IMPORT_JOB_H
