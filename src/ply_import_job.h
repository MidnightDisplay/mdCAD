//------------------------------------------------------------------------------
// ply_import_job.h - PLY import job state machine (header-only)
//
// Provides chunked PLY import with progress tracking:
// - Parses PLY file in chunks across frames
// - Creates entities in chunks for Editable Subtree mode
// - Progress callback for UI updates
// - Cancellation support
//------------------------------------------------------------------------------
#ifndef PLY_IMPORT_JOB_H
#define PLY_IMPORT_JOB_H

#include "ply_loader.h"
#include "ecs/ecs_scene.h"
#include <string.h>

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------

// Chunk sizes (tunable for performance)
#define PLY_PARSE_CHUNK_SIZE      5000    // Vertices per frame during parsing
#define PLY_ENTITY_CHUNK_SIZE     500     // Entities per frame during creation

// Threshold for synchronous import (skip progress UI for small files)
#define PLY_SYNC_THRESHOLD        1000    // Points below this import synchronously

//------------------------------------------------------------------------------
// Job states
//------------------------------------------------------------------------------

typedef enum {
    PLY_JOB_IDLE = 0,           // No job running
    PLY_JOB_PARSING_VERTICES,   // Parsing PLY vertex data
    PLY_JOB_CREATING_ENTITIES,  // Creating ECS entities (Editable mode only)
    PLY_JOB_COMPLETE,           // Job finished successfully
    PLY_JOB_CANCELLED,          // Job was cancelled
    PLY_JOB_ERROR               // Job failed with error
} ply_job_state_t;

//------------------------------------------------------------------------------
// Import job structure
//------------------------------------------------------------------------------

typedef struct {
    ply_job_state_t state;

    // File parsing state
    ply_parse_state_t parse_state;
    char filepath[512];

    // Import options
    int import_mode;            // 0 = Point Cloud Node, 1 = Editable Subtree
    float scale;                // Unit conversion scale factor
    float point_size;           // Point size for rendering
    vec4_t default_color;       // Color when not using PLY colors
    bool use_ply_colors;        // Whether to use colors from PLY file

    // Entity creation state (for Editable Subtree mode)
    int created_count;          // Number of entities created so far
    ecs_entity_t root_entity;   // Root entity for Editable Subtree

    // Progress (0.0 - 1.0)
    float progress;
    char status_message[128];

    // Result
    ply_error_t error;
    int total_points;           // Total points imported

} ply_import_job_t;

//------------------------------------------------------------------------------
// Initialize job structure
//------------------------------------------------------------------------------

static inline void ply_import_job_init(ply_import_job_t *job) {
    memset(job, 0, sizeof(ply_import_job_t));
    job->state = PLY_JOB_IDLE;
    job->scale = 1.0f;
    job->point_size = 0.01f;
    job->default_color = vec4_make(1.0f, 1.0f, 1.0f, 1.0f);
    job->use_ply_colors = true;
}

//------------------------------------------------------------------------------
// Start a new import job
//------------------------------------------------------------------------------

static inline bool ply_import_job_start(ply_import_job_t *job,
                                         const char *filepath,
                                         int import_mode,
                                         float scale,
                                         float point_size,
                                         vec4_t default_color,
                                         bool use_ply_colors) {
    // Cancel any running job first
    if (job->state != PLY_JOB_IDLE &&
        job->state != PLY_JOB_COMPLETE &&
        job->state != PLY_JOB_CANCELLED &&
        job->state != PLY_JOB_ERROR) {
        ply_parse_state_free(&job->parse_state);
    }

    // Store options
    strncpy(job->filepath, filepath, sizeof(job->filepath) - 1);
    job->filepath[sizeof(job->filepath) - 1] = '\0';
    job->import_mode = import_mode;
    job->scale = scale;
    job->point_size = point_size;
    job->default_color = default_color;
    job->use_ply_colors = use_ply_colors;

    // Reset state
    job->created_count = 0;
    job->root_entity = 0;
    job->progress = 0.0f;
    job->error = PLY_OK;
    job->total_points = 0;

    // Open file and parse header
    ply_error_t err = ply_open(filepath, &job->parse_state);
    if (err != PLY_OK) {
        job->state = PLY_JOB_ERROR;
        job->error = err;
        snprintf(job->status_message, sizeof(job->status_message),
                 "Error: %s", ply_error_string(err));
        return false;
    }

    job->total_points = job->parse_state.header.vertex_count;
    job->state = PLY_JOB_PARSING_VERTICES;
    snprintf(job->status_message, sizeof(job->status_message),
             "Parsing: 0 / %d vertices", job->total_points);

    return true;
}

//------------------------------------------------------------------------------
// Check if job should use synchronous import (small file)
//------------------------------------------------------------------------------

static inline bool ply_import_job_should_sync(const ply_import_job_t *job) {
    return job->total_points < PLY_SYNC_THRESHOLD;
}

//------------------------------------------------------------------------------
// Process one chunk of work - returns true when job is complete
//------------------------------------------------------------------------------

static inline bool ply_import_job_tick(ply_import_job_t *job, ecs_scene_t *scene) {
    if (job->state == PLY_JOB_IDLE ||
        job->state == PLY_JOB_COMPLETE ||
        job->state == PLY_JOB_CANCELLED ||
        job->state == PLY_JOB_ERROR) {
        return true;  // Already complete
    }

    //--------------------------------------------------------------------------
    // State: PARSING_VERTICES
    //--------------------------------------------------------------------------
    if (job->state == PLY_JOB_PARSING_VERTICES) {
        int parsed = ply_parse_vertices_chunk(&job->parse_state, PLY_PARSE_CHUNK_SIZE);

        if (job->parse_state.error != PLY_OK) {
            job->state = PLY_JOB_ERROR;
            job->error = job->parse_state.error;
            snprintf(job->status_message, sizeof(job->status_message),
                     "Parse error: %s", ply_error_string(job->error));
            ply_parse_state_free(&job->parse_state);
            return true;
        }

        // Update progress - parsing is 30% for Editable mode, 95% for Point Cloud mode
        float parse_weight = (job->import_mode == 0) ? 0.95f : 0.30f;
        job->progress = parse_weight * ply_get_progress(&job->parse_state);

        snprintf(job->status_message, sizeof(job->status_message),
                 "Parsing: %d / %d vertices",
                 job->parse_state.parsed_count, job->total_points);

        // Check if parsing is complete
        if (ply_is_complete(&job->parse_state)) {
            ply_close(&job->parse_state);  // Close file, keep data

            if (job->import_mode == 0) {
                // Point Cloud Node mode - create single entity
                vec4_t *colors = NULL;
                if (job->use_ply_colors && job->parse_state.has_colors) {
                    colors = job->parse_state.colors;
                }

                ecs_entity_t cloud = scene_add_point_cloud(
                    scene,
                    job->parse_state.points,
                    colors,
                    job->parse_state.parsed_count,
                    job->default_color,
                    job->point_size
                );

                // Apply scale via transform
                if (cloud != 0 && job->scale != 1.0f) {
                    TransformComp *t = ecs_world_get_transform(scene->world, cloud);
                    if (t) {
                        t->scale = vec3_make(job->scale, job->scale, job->scale);
                        t->dirty = true;
                    }
                    RenderableComp *r = ecs_world_get_renderable(scene->world, cloud);
                    if (r) {
                        r->instance_dirty = true;
                    }
                }

                // Free parsed data
                ply_parse_state_free(&job->parse_state);

                job->progress = 1.0f;
                job->state = PLY_JOB_COMPLETE;
                snprintf(job->status_message, sizeof(job->status_message),
                         "Imported %d points as Point Cloud", job->total_points);
                return true;
            } else {
                // Editable Subtree mode - create root entity first
                job->root_entity = scene_add_point(
                    scene,
                    vec3_make(0, 0, 0),
                    job->default_color,
                    job->point_size
                );

                // Apply scale to root
                if (job->root_entity != 0 && job->scale != 1.0f) {
                    TransformComp *t = ecs_world_get_transform(scene->world, job->root_entity);
                    if (t) {
                        t->scale = vec3_make(job->scale, job->scale, job->scale);
                        t->dirty = true;
                    }
                }

                job->created_count = 0;
                job->state = PLY_JOB_CREATING_ENTITIES;
                snprintf(job->status_message, sizeof(job->status_message),
                         "Creating: 0 / %d entities", job->total_points);
            }
        }

        return false;  // Not complete yet
    }

    //--------------------------------------------------------------------------
    // State: CREATING_ENTITIES (Editable Subtree mode)
    //--------------------------------------------------------------------------
    if (job->state == PLY_JOB_CREATING_ENTITIES) {
        int remaining = job->total_points - job->created_count;
        int to_create = (remaining < PLY_ENTITY_CHUNK_SIZE) ? remaining : PLY_ENTITY_CHUNK_SIZE;

        vec4_t *colors = NULL;
        if (job->use_ply_colors && job->parse_state.has_colors) {
            colors = job->parse_state.colors;
        }

        for (int i = 0; i < to_create; i++) {
            int idx = job->created_count + i;
            vec4_t pt_color = colors ? colors[idx] : job->default_color;
            ecs_entity_t pt = scene_add_point(
                scene,
                job->parse_state.points[idx],
                pt_color,
                job->point_size
            );
            if (pt != 0) {
                scene_set_parent(scene, pt, job->root_entity);
            }
        }

        job->created_count += to_create;

        // Update progress - entity creation is 30-100% (70% of total)
        float entity_progress = (float)job->created_count / (float)job->total_points;
        job->progress = 0.30f + 0.70f * entity_progress;

        snprintf(job->status_message, sizeof(job->status_message),
                 "Creating: %d / %d entities",
                 job->created_count, job->total_points);

        // Check if complete
        if (job->created_count >= job->total_points) {
            // Free parsed data
            ply_parse_state_free(&job->parse_state);

            job->progress = 1.0f;
            job->state = PLY_JOB_COMPLETE;
            snprintf(job->status_message, sizeof(job->status_message),
                     "Imported %d points as Editable Subtree", job->total_points);
            return true;
        }

        return false;  // Not complete yet
    }

    return true;  // Unknown state, consider complete
}

//------------------------------------------------------------------------------
// Cancel an in-progress job
//------------------------------------------------------------------------------

static inline void ply_import_job_cancel(ply_import_job_t *job, ecs_scene_t *scene) {
    if (job->state == PLY_JOB_IDLE ||
        job->state == PLY_JOB_COMPLETE ||
        job->state == PLY_JOB_CANCELLED ||
        job->state == PLY_JOB_ERROR) {
        return;  // Nothing to cancel
    }

    // Free parsed data
    ply_parse_state_free(&job->parse_state);

    // If we were creating entities, delete the partial root entity and its children
    if (job->state == PLY_JOB_CREATING_ENTITIES && job->root_entity != 0) {
        scene_remove_entity(scene, job->root_entity);
        job->root_entity = 0;
    }

    job->state = PLY_JOB_CANCELLED;
    job->progress = 0.0f;
    snprintf(job->status_message, sizeof(job->status_message), "Import cancelled");
}

//------------------------------------------------------------------------------
// Check if job is currently running
//------------------------------------------------------------------------------

static inline bool ply_import_job_is_running(const ply_import_job_t *job) {
    return job->state == PLY_JOB_PARSING_VERTICES ||
           job->state == PLY_JOB_CREATING_ENTITIES;
}

//------------------------------------------------------------------------------
// Reset job to idle state
//------------------------------------------------------------------------------

static inline void ply_import_job_reset(ply_import_job_t *job) {
    if (ply_import_job_is_running(job)) {
        ply_parse_state_free(&job->parse_state);
    }
    job->state = PLY_JOB_IDLE;
    job->progress = 0.0f;
    job->status_message[0] = '\0';
}

#endif // PLY_IMPORT_JOB_H
