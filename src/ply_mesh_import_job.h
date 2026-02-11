//------------------------------------------------------------------------------
// ply_mesh_import_job.h - PLY mesh import job state machine (header-only)
//
// Provides chunked PLY mesh import with progress tracking:
// - Parses PLY file vertices and faces in chunks across frames
// - Two import modes: Single Mesh Entity or Individual Triangles
// - Progress callback for UI updates
// - Cancellation support
// - Import transformations: CoM shift, rotation, scale
//------------------------------------------------------------------------------
#ifndef PLY_MESH_IMPORT_JOB_H
#define PLY_MESH_IMPORT_JOB_H

#include "ply_loader.h"
#include "ecs/ecs_scene.h"
#include "sokol_time.h"
#include <string.h>

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------

// Chunk sizes (tunable for performance)
#define PLY_MESH_PARSE_CHUNK_SIZE     5000    // Vertices/faces per frame during parsing
#define PLY_MESH_ENTITY_CHUNK_SIZE    500     // Entities per frame during creation

// Threshold for synchronous import (skip progress UI for small files)
#define PLY_MESH_SYNC_THRESHOLD       1000    // Faces below this import synchronously

//------------------------------------------------------------------------------
// Job states
//------------------------------------------------------------------------------

typedef enum {
    PLY_MESH_JOB_IDLE = 0,           // No job running
    PLY_MESH_JOB_PARSING_VERTICES,   // Parsing PLY vertex data
    PLY_MESH_JOB_PARSING_FACES,      // Parsing PLY face data
    PLY_MESH_JOB_CREATING_ENTITIES,  // Creating ECS entities (Individual mode only)
    PLY_MESH_JOB_COMPLETE,           // Job finished successfully
    PLY_MESH_JOB_CANCELLED,          // Job was cancelled
    PLY_MESH_JOB_ERROR               // Job failed with error
} ply_mesh_job_state_t;

//------------------------------------------------------------------------------
// Import job structure
//------------------------------------------------------------------------------

// Maximum number of timing samples for the speed plot
#define PLY_MESH_JOB_MAX_TIMING_SAMPLES 100

typedef struct {
    ply_mesh_job_state_t state;

    // File parsing state
    ply_parse_state_t parse_state;
    char filepath[512];

    // Import options
    int import_mode;            // 0 = Single Mesh Entity, 1 = Individual Triangles
    float scale;                // Unit conversion scale factor
    vec4_t default_color;       // Color when not using PLY colors
    bool use_ply_colors;        // Whether to use colors from PLY file

    // Import transformation options (applied to coordinates)
    bool shift_to_com;          // Shift points so centre of mass is at origin
    float rotation_x;           // Rotation around X axis (radians)
    float rotation_y;           // Rotation around Y axis (radians)
    float rotation_z;           // Rotation around Z axis (radians)

    // Computed transformation data (computed after parsing, before entity creation)
    vec3_t com;                 // Centre of mass (computed if shift_to_com is true)
    mat4_t transform_matrix;    // Combined rotation matrix (computed once)
    bool transforms_applied;    // Whether transforms have been applied to parsed data

    // Entity creation state (for Individual Triangles mode)
    int created_count;          // Number of triangles created so far

    // Progress (0.0 - 1.0)
    float progress;
    char status_message[128];

    // Timing data for iteration speed tracking
    double last_iteration_time_ms;
    float iteration_times[PLY_MESH_JOB_MAX_TIMING_SAMPLES];
    float iteration_progress[PLY_MESH_JOB_MAX_TIMING_SAMPLES];
    int timing_sample_count;
    uint64_t iteration_start_time;
    float last_sampled_progress;

    // Result
    ply_error_t error;
    int total_vertices;
    int total_faces;

} ply_mesh_import_job_t;

//------------------------------------------------------------------------------
// Initialize job structure
//------------------------------------------------------------------------------

static inline void ply_mesh_import_job_init(ply_mesh_import_job_t *job) {
    memset(job, 0, sizeof(ply_mesh_import_job_t));
    job->state = PLY_MESH_JOB_IDLE;
    job->scale = 1.0f;
    job->default_color = vec4_make(0.7f, 0.7f, 0.7f, 1.0f);
    job->use_ply_colors = true;
    job->shift_to_com = false;
    job->rotation_x = 0.0f;
    job->rotation_y = 0.0f;
    job->rotation_z = 0.0f;
    job->transforms_applied = false;
    job->last_iteration_time_ms = 0.0;
    job->timing_sample_count = 0;
    job->iteration_start_time = 0;
    job->last_sampled_progress = -1.0f;
}

//------------------------------------------------------------------------------
// Start a new import job
//------------------------------------------------------------------------------

static inline bool ply_mesh_import_job_start(ply_mesh_import_job_t *job,
                                              const char *filepath,
                                              int import_mode,
                                              float scale,
                                              vec4_t default_color,
                                              bool use_ply_colors,
                                              bool shift_to_com,
                                              float rotation_x,
                                              float rotation_y,
                                              float rotation_z) {
    // Cancel any running job first
    if (job->state != PLY_MESH_JOB_IDLE &&
        job->state != PLY_MESH_JOB_COMPLETE &&
        job->state != PLY_MESH_JOB_CANCELLED &&
        job->state != PLY_MESH_JOB_ERROR) {
        ply_parse_state_free(&job->parse_state);
    }

    // Store options
    strncpy(job->filepath, filepath, sizeof(job->filepath) - 1);
    job->filepath[sizeof(job->filepath) - 1] = '\0';
    job->import_mode = import_mode;
    job->scale = scale;
    job->default_color = default_color;
    job->use_ply_colors = use_ply_colors;
    job->shift_to_com = shift_to_com;
    job->rotation_x = rotation_x;
    job->rotation_y = rotation_y;
    job->rotation_z = rotation_z;

    // Reset state
    job->created_count = 0;
    job->progress = 0.0f;
    job->error = PLY_OK;
    job->total_vertices = 0;
    job->total_faces = 0;
    job->transforms_applied = false;
    job->com = vec3_make(0, 0, 0);
    job->transform_matrix = mat4_identity();

    // Reset timing data
    job->last_iteration_time_ms = 0.0;
    job->timing_sample_count = 0;
    job->iteration_start_time = 0;
    job->last_sampled_progress = -1.0f;

    // Open file and parse header
    ply_error_t err = ply_open(filepath, &job->parse_state);
    if (err != PLY_OK) {
        job->state = PLY_MESH_JOB_ERROR;
        job->error = err;
        snprintf(job->status_message, sizeof(job->status_message),
                 "Error: %s", ply_error_string(err));
        return false;
    }

    job->total_vertices = job->parse_state.header.vertex_count;
    job->total_faces = job->parse_state.face_total;

    // Validate that this file actually has faces
    if (job->total_faces <= 0) {
        ply_parse_state_free(&job->parse_state);
        job->state = PLY_MESH_JOB_ERROR;
        job->error = PLY_ERROR_PARSE_ERROR;
        snprintf(job->status_message, sizeof(job->status_message),
                 "Error: PLY file has no face data (use Point Cloud import instead)");
        return false;
    }

    job->state = PLY_MESH_JOB_PARSING_VERTICES;
    snprintf(job->status_message, sizeof(job->status_message),
             "Parsing: 0 / %d vertices", job->total_vertices);

    // Record initial 0% sample for the plot
    job->iteration_times[0] = 0.0f;
    job->iteration_progress[0] = 0.0f;
    job->timing_sample_count = 1;

    return true;
}

//------------------------------------------------------------------------------
// Check if job should use synchronous import (small file)
//------------------------------------------------------------------------------

static inline bool ply_mesh_import_job_should_sync(const ply_mesh_import_job_t *job) {
    return job->total_faces < PLY_MESH_SYNC_THRESHOLD;
}

//------------------------------------------------------------------------------
// Internal: Apply transformations to parsed vertex data
// Order: 1) CoM shift, 2) Rotation (X->Y->Z), 3) Scale
//------------------------------------------------------------------------------

static inline void ply_mesh_import_job_apply_transforms(ply_mesh_import_job_t *job) {
    if (job->transforms_applied) return;

    int count = job->parse_state.parsed_count;
    vec3_t *points = job->parse_state.points;

    // Step 1: Calculate and apply Centre of Mass shift
    if (job->shift_to_com && count > 0) {
        vec3_t sum = vec3_make(0, 0, 0);
        for (int i = 0; i < count; i++) {
            sum = vec3_add(sum, points[i]);
        }
        job->com = vec3_scale(sum, 1.0f / (float)count);

        for (int i = 0; i < count; i++) {
            points[i] = vec3_sub(points[i], job->com);
        }
    }

    // Step 2: Build rotation matrix (X -> Y -> Z order)
    bool has_rotation = (job->rotation_x != 0.0f ||
                         job->rotation_y != 0.0f ||
                         job->rotation_z != 0.0f);

    if (has_rotation) {
        mat4_t rot_x = mat4_rotate_x(job->rotation_x);
        mat4_t rot_y = mat4_rotate_y(job->rotation_y);
        mat4_t rot_z = mat4_rotate_z(job->rotation_z);
        mat4_t rot_xy = mat4_mul(rot_y, rot_x);
        job->transform_matrix = mat4_mul(rot_z, rot_xy);

        for (int i = 0; i < count; i++) {
            points[i] = mat4_mul_point(job->transform_matrix, points[i]);
        }
    }

    // Step 3: Apply scale to all points
    if (job->scale != 1.0f) {
        for (int i = 0; i < count; i++) {
            points[i] = vec3_scale(points[i], job->scale);
        }
    }

    job->transforms_applied = true;
}

//------------------------------------------------------------------------------
// Internal: Convert per-face colors to per-vertex colors by averaging
// When a PLY file has face colors but no vertex colors, we need to convert
// them to per-vertex colors for the mesh entity API.
//------------------------------------------------------------------------------

static inline vec4_t* ply_mesh_face_colors_to_vertex_colors(
    const uint32_t *indices, int tri_count,
    const vec4_t *face_colors, int face_count,
    int vertex_count)
{
    (void)face_count;  // face_count == tri_count for triangulated data

    vec4_t *vertex_colors = (vec4_t*)calloc(vertex_count, sizeof(vec4_t));
    int *vertex_face_counts = (int*)calloc(vertex_count, sizeof(int));

    if (!vertex_colors || !vertex_face_counts) {
        if (vertex_colors) free(vertex_colors);
        if (vertex_face_counts) free(vertex_face_counts);
        return NULL;
    }

    // Accumulate face colors at each vertex
    for (int t = 0; t < tri_count; t++) {
        vec4_t fc = face_colors[t];
        for (int v = 0; v < 3; v++) {
            uint32_t vi = indices[t * 3 + v];
            if ((int)vi < vertex_count) {
                vertex_colors[vi].x += fc.x;
                vertex_colors[vi].y += fc.y;
                vertex_colors[vi].z += fc.z;
                vertex_colors[vi].w += fc.w;
                vertex_face_counts[vi]++;
            }
        }
    }

    // Average
    for (int i = 0; i < vertex_count; i++) {
        if (vertex_face_counts[i] > 0) {
            float inv = 1.0f / (float)vertex_face_counts[i];
            vertex_colors[i].x *= inv;
            vertex_colors[i].y *= inv;
            vertex_colors[i].z *= inv;
            vertex_colors[i].w *= inv;
        } else {
            vertex_colors[i] = vec4_make(0.7f, 0.7f, 0.7f, 1.0f);
        }
    }

    free(vertex_face_counts);
    return vertex_colors;
}

//------------------------------------------------------------------------------
// Process one chunk of work - returns true when job is complete
//------------------------------------------------------------------------------

static inline bool ply_mesh_import_job_tick(ply_mesh_import_job_t *job, ecs_scene_t *scene) {
    if (job->state == PLY_MESH_JOB_IDLE ||
        job->state == PLY_MESH_JOB_COMPLETE ||
        job->state == PLY_MESH_JOB_CANCELLED ||
        job->state == PLY_MESH_JOB_ERROR) {
        return true;
    }

    //--------------------------------------------------------------------------
    // State: PARSING_VERTICES
    //--------------------------------------------------------------------------
    if (job->state == PLY_MESH_JOB_PARSING_VERTICES) {
        uint64_t iter_start = stm_now();

        int parsed = ply_parse_vertices_chunk(&job->parse_state, PLY_MESH_PARSE_CHUNK_SIZE);
        (void)parsed;

        uint64_t iter_end = stm_now();
        job->last_iteration_time_ms = stm_ms(stm_diff(iter_end, iter_start));

        if (job->parse_state.error != PLY_OK) {
            job->state = PLY_MESH_JOB_ERROR;
            job->error = job->parse_state.error;
            snprintf(job->status_message, sizeof(job->status_message),
                     "Parse error: %s", ply_error_string(job->error));
            ply_parse_state_free(&job->parse_state);
            return true;
        }

        // Progress: vertex parsing is ~30% of total for both modes
        float vertex_frac = (float)job->parse_state.parsed_count / (float)job->total_vertices;
        job->progress = 0.30f * vertex_frac;

        // Store timing sample
        float progress_pct = job->progress * 100.0f;
        if (job->timing_sample_count < PLY_MESH_JOB_MAX_TIMING_SAMPLES &&
            (progress_pct - job->last_sampled_progress) >= 1.0f) {
            job->iteration_times[job->timing_sample_count] = (float)job->last_iteration_time_ms;
            job->iteration_progress[job->timing_sample_count] = progress_pct;
            job->timing_sample_count++;
            job->last_sampled_progress = progress_pct;
        }

        snprintf(job->status_message, sizeof(job->status_message),
                 "Parsing: %d / %d vertices",
                 job->parse_state.parsed_count, job->total_vertices);

        // Transition to face parsing when vertices complete
        if (ply_vertices_complete(&job->parse_state)) {
            job->state = PLY_MESH_JOB_PARSING_FACES;
            snprintf(job->status_message, sizeof(job->status_message),
                     "Parsing: 0 / %d faces", job->total_faces);
        }

        return false;
    }

    //--------------------------------------------------------------------------
    // State: PARSING_FACES
    //--------------------------------------------------------------------------
    if (job->state == PLY_MESH_JOB_PARSING_FACES) {
        uint64_t iter_start = stm_now();

        int parsed = ply_parse_faces_chunk(&job->parse_state, PLY_MESH_PARSE_CHUNK_SIZE);
        (void)parsed;

        uint64_t iter_end = stm_now();
        job->last_iteration_time_ms = stm_ms(stm_diff(iter_end, iter_start));

        if (job->parse_state.error != PLY_OK) {
            job->state = PLY_MESH_JOB_ERROR;
            job->error = job->parse_state.error;
            snprintf(job->status_message, sizeof(job->status_message),
                     "Face parse error: %s", ply_error_string(job->error));
            ply_parse_state_free(&job->parse_state);
            return true;
        }

        // Progress: face parsing is 30%-60% (another 30%)
        float face_frac = (job->total_faces > 0) ?
            (float)job->parse_state.face_parsed_count / (float)job->total_faces : 1.0f;
        job->progress = 0.30f + 0.30f * face_frac;

        // Store timing sample
        float progress_pct = job->progress * 100.0f;
        if (job->timing_sample_count < PLY_MESH_JOB_MAX_TIMING_SAMPLES &&
            (progress_pct - job->last_sampled_progress) >= 1.0f) {
            job->iteration_times[job->timing_sample_count] = (float)job->last_iteration_time_ms;
            job->iteration_progress[job->timing_sample_count] = progress_pct;
            job->timing_sample_count++;
            job->last_sampled_progress = progress_pct;
        }

        snprintf(job->status_message, sizeof(job->status_message),
                 "Parsing: %d / %d faces",
                 job->parse_state.face_parsed_count, job->total_faces);

        // Check if face parsing is complete
        if (job->parse_state.face_parsed_count >= job->total_faces) {
            ply_close(&job->parse_state);

            // Apply all transformations to parsed vertex data
            ply_mesh_import_job_apply_transforms(job);

            if (job->import_mode == 0) {
                // ---- Single Mesh Entity mode ----
                // Transfer parsed data to mesh_data
                ply_mesh_data_t mesh;
                ply_parse_state_to_mesh_data(&job->parse_state, &mesh);

                int tri_count = mesh.faces.tri_count;
                int index_count = tri_count * 3;

                // Determine which color source to use
                bool has_vertex_colors = job->use_ply_colors && mesh.has_vertex_colors;
                bool has_face_colors = job->use_ply_colors && mesh.has_face_colors && !has_vertex_colors;

                ecs_entity_t entity = 0;

                if (has_vertex_colors) {
                    entity = scene_add_mesh_colored(
                        scene,
                        mesh.vertices, mesh.vertex_count,
                        mesh.faces.indices, index_count,
                        mesh.vertex_colors
                    );
                } else if (has_face_colors) {
                    // Convert face colors to vertex colors
                    vec4_t *vert_colors = ply_mesh_face_colors_to_vertex_colors(
                        mesh.faces.indices, tri_count,
                        mesh.faces.colors, mesh.faces.face_count,
                        mesh.vertex_count
                    );

                    if (vert_colors) {
                        entity = scene_add_mesh_colored(
                            scene,
                            mesh.vertices, mesh.vertex_count,
                            mesh.faces.indices, index_count,
                            vert_colors
                        );
                        free(vert_colors);
                    } else {
                        entity = scene_add_mesh(
                            scene,
                            mesh.vertices, mesh.vertex_count,
                            mesh.faces.indices, index_count,
                            job->default_color
                        );
                    }
                } else {
                    entity = scene_add_mesh(
                        scene,
                        mesh.vertices, mesh.vertex_count,
                        mesh.faces.indices, index_count,
                        job->default_color
                    );
                }

                ply_mesh_data_free(&mesh);

                if (entity == 0) {
                    job->state = PLY_MESH_JOB_ERROR;
                    job->error = PLY_ERROR_MEMORY_ALLOCATION;
                    snprintf(job->status_message, sizeof(job->status_message),
                             "Slot buffer limit reached. Clear scene and try again.");
                    return true;
                }

                job->progress = 1.0f;
                job->state = PLY_MESH_JOB_COMPLETE;

                if (job->timing_sample_count < PLY_MESH_JOB_MAX_TIMING_SAMPLES) {
                    job->iteration_times[job->timing_sample_count] = (float)job->last_iteration_time_ms;
                    job->iteration_progress[job->timing_sample_count] = 100.0f;
                    job->timing_sample_count++;
                }

                snprintf(job->status_message, sizeof(job->status_message),
                         "Imported %d vertices, %d faces as Single Mesh",
                         job->total_vertices, tri_count);
                return true;
            } else {
                // ---- Individual Triangles mode ----
                job->created_count = 0;
                job->state = PLY_MESH_JOB_CREATING_ENTITIES;
                snprintf(job->status_message, sizeof(job->status_message),
                         "Creating: 0 / %d triangles",
                         job->parse_state.face_tri_count);
            }
        }

        return false;
    }

    //--------------------------------------------------------------------------
    // State: CREATING_ENTITIES (Individual Triangles mode)
    //--------------------------------------------------------------------------
    if (job->state == PLY_MESH_JOB_CREATING_ENTITIES) {
        uint64_t iter_start = stm_now();

        int total_tris = job->parse_state.face_tri_count;
        int remaining = total_tris - job->created_count;
        int to_create = (remaining < PLY_MESH_ENTITY_CHUNK_SIZE) ? remaining : PLY_MESH_ENTITY_CHUNK_SIZE;

        vec3_t *verts = job->parse_state.points;
        uint32_t *indices = job->parse_state.face_indices;
        vec4_t *vert_colors = (job->use_ply_colors && job->parse_state.has_colors) ?
                               job->parse_state.colors : NULL;
        vec4_t *face_colors = (job->use_ply_colors && job->parse_state.has_face_colors &&
                               !vert_colors) ? job->parse_state.face_colors : NULL;

        int actually_created = 0;
        for (int i = 0; i < to_create; i++) {
            int tri_idx = job->created_count + i;
            uint32_t i0 = indices[tri_idx * 3 + 0];
            uint32_t i1 = indices[tri_idx * 3 + 1];
            uint32_t i2 = indices[tri_idx * 3 + 2];

            vec3_t a = verts[i0];
            vec3_t b = verts[i1];
            vec3_t c = verts[i2];

            ecs_entity_t e = 0;

            if (vert_colors) {
                e = scene_add_triangle_colored(scene, a, b, c,
                                               vert_colors[i0], vert_colors[i1], vert_colors[i2]);
            } else if (face_colors) {
                vec4_t fc = face_colors[tri_idx];
                e = scene_add_triangle_colored(scene, a, b, c, fc, fc, fc);
            } else {
                e = scene_add_triangle(scene, a, b, c, job->default_color);
            }

            if (e == 0) {
                ply_parse_state_free(&job->parse_state);
                job->state = PLY_MESH_JOB_ERROR;
                job->error = PLY_ERROR_MEMORY_ALLOCATION;
                snprintf(job->status_message, sizeof(job->status_message),
                         "Slot buffer limit reached at %d/%d triangles. Clear scene and try again.",
                         job->created_count + actually_created, total_tris);
                return true;
            }
            actually_created++;
        }

        job->created_count += actually_created;

        uint64_t iter_end = stm_now();
        job->last_iteration_time_ms = stm_ms(stm_diff(iter_end, iter_start));

        // Progress: entity creation is 60%-100% (40% of total)
        float entity_progress = (float)job->created_count / (float)total_tris;
        job->progress = 0.60f + 0.40f * entity_progress;

        // Store timing sample
        float progress_pct = job->progress * 100.0f;
        if (job->timing_sample_count < PLY_MESH_JOB_MAX_TIMING_SAMPLES &&
            (progress_pct - job->last_sampled_progress) >= 1.0f) {
            job->iteration_times[job->timing_sample_count] = (float)job->last_iteration_time_ms;
            job->iteration_progress[job->timing_sample_count] = progress_pct;
            job->timing_sample_count++;
            job->last_sampled_progress = progress_pct;
        }

        snprintf(job->status_message, sizeof(job->status_message),
                 "Creating: %d / %d triangles",
                 job->created_count, total_tris);

        if (job->created_count >= total_tris) {
            ply_parse_state_free(&job->parse_state);

            job->progress = 1.0f;
            job->state = PLY_MESH_JOB_COMPLETE;

            if (job->timing_sample_count < PLY_MESH_JOB_MAX_TIMING_SAMPLES) {
                job->iteration_times[job->timing_sample_count] = (float)job->last_iteration_time_ms;
                job->iteration_progress[job->timing_sample_count] = 100.0f;
                job->timing_sample_count++;
            }

            snprintf(job->status_message, sizeof(job->status_message),
                     "Imported %d triangles as Individual Entities",
                     total_tris);
            return true;
        }

        return false;
    }

    return true;  // Unknown state, consider complete
}

//------------------------------------------------------------------------------
// Cancel an in-progress job
//------------------------------------------------------------------------------

static inline void ply_mesh_import_job_cancel(ply_mesh_import_job_t *job, ecs_scene_t *scene) {
    (void)scene;

    if (job->state == PLY_MESH_JOB_IDLE ||
        job->state == PLY_MESH_JOB_COMPLETE ||
        job->state == PLY_MESH_JOB_CANCELLED ||
        job->state == PLY_MESH_JOB_ERROR) {
        return;
    }

    ply_parse_state_free(&job->parse_state);

    job->state = PLY_MESH_JOB_CANCELLED;
    job->progress = 0.0f;
    snprintf(job->status_message, sizeof(job->status_message), "Import cancelled");
}

//------------------------------------------------------------------------------
// Check if job is currently running
//------------------------------------------------------------------------------

static inline bool ply_mesh_import_job_is_running(const ply_mesh_import_job_t *job) {
    return job->state == PLY_MESH_JOB_PARSING_VERTICES ||
           job->state == PLY_MESH_JOB_PARSING_FACES ||
           job->state == PLY_MESH_JOB_CREATING_ENTITIES;
}

//------------------------------------------------------------------------------
// Reset job to idle state
//------------------------------------------------------------------------------

static inline void ply_mesh_import_job_reset(ply_mesh_import_job_t *job) {
    if (ply_mesh_import_job_is_running(job)) {
        ply_parse_state_free(&job->parse_state);
    }
    job->state = PLY_MESH_JOB_IDLE;
    job->progress = 0.0f;
    job->status_message[0] = '\0';
    job->transforms_applied = false;
}

#endif // PLY_MESH_IMPORT_JOB_H
