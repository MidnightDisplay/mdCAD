//------------------------------------------------------------------------------
// ply_import_job.h - PLY import job state machine (header-only)
//
// Provides chunked PLY import with progress tracking:
// - Parses PLY file in chunks across frames
// - Creates entities in chunks for Editable Subtree mode
// - Progress callback for UI updates
// - Cancellation support
// - Import transformations: CoM shift, rotation, scale
//------------------------------------------------------------------------------
#ifndef PLY_IMPORT_JOB_H
#define PLY_IMPORT_JOB_H

#include "ply_loader.h"
#include "ecs/ecs_scene.h"
#include "sokol_time.h"
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

// Maximum number of timing samples for the speed plot
#define PLY_JOB_MAX_TIMING_SAMPLES 100

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

    // Import transformation options (applied to coordinates)
    bool shift_to_com;          // Shift points so centre of mass is at origin
    float rotation_x;           // Rotation around X axis (radians)
    float rotation_y;           // Rotation around Y axis (radians)
    float rotation_z;           // Rotation around Z axis (radians)

    // Computed transformation data (computed after parsing, before entity creation)
    vec3_t com;                 // Centre of mass (computed if shift_to_com is true)
    mat4_t transform_matrix;    // Combined rotation matrix (computed once)
    bool transforms_applied;    // Whether transforms have been applied to parsed data

    // Entity creation state (for Editable Subtree mode)
    int created_count;          // Number of entities created so far

    // Progress (0.0 - 1.0)
    float progress;
    char status_message[128];

    // Timing data for iteration speed tracking
    double last_iteration_time_ms;      // Time of last iteration in milliseconds
    float iteration_times[PLY_JOB_MAX_TIMING_SAMPLES];  // Ring buffer of iteration times
    float iteration_progress[PLY_JOB_MAX_TIMING_SAMPLES]; // Progress % at each sample
    int timing_sample_count;            // Number of samples collected
    uint64_t iteration_start_time;      // Start time of current iteration (stm_now())
    float last_sampled_progress;        // Last progress value that was sampled (for even distribution)

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
    job->shift_to_com = false;
    job->rotation_x = 0.0f;
    job->rotation_y = 0.0f;
    job->rotation_z = 0.0f;
    job->transforms_applied = false;
    job->last_iteration_time_ms = 0.0;
    job->timing_sample_count = 0;
    job->iteration_start_time = 0;
    job->last_sampled_progress = -1.0f;  // Force first sample
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
                                         bool use_ply_colors,
                                         bool shift_to_com,
                                         float rotation_x,
                                         float rotation_y,
                                         float rotation_z) {
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
    job->shift_to_com = shift_to_com;
    job->rotation_x = rotation_x;
    job->rotation_y = rotation_y;
    job->rotation_z = rotation_z;

    // Reset state
    job->created_count = 0;
    job->progress = 0.0f;
    job->error = PLY_OK;
    job->total_points = 0;
    job->transforms_applied = false;
    job->com = vec3_make(0, 0, 0);
    job->transform_matrix = mat4_identity();

    // Reset timing data
    job->last_iteration_time_ms = 0.0;
    job->timing_sample_count = 0;
    job->iteration_start_time = 0;
    job->last_sampled_progress = -1.0f;  // Force first sample

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

    // Record initial 0% sample for the plot
    job->iteration_times[0] = 0.0f;
    job->iteration_progress[0] = 0.0f;
    job->timing_sample_count = 1;

    return true;
}

//------------------------------------------------------------------------------
// Check if job should use synchronous import (small file)
//------------------------------------------------------------------------------

static inline bool ply_import_job_should_sync(const ply_import_job_t *job) {
    return job->total_points < PLY_SYNC_THRESHOLD;
}

//------------------------------------------------------------------------------
// Internal: Apply transformations to parsed point data
// Order: 1) CoM shift, 2) Rotation (X->Y->Z), 3) Scale
//------------------------------------------------------------------------------

static inline void ply_import_job_apply_transforms(ply_import_job_t *job) {
    if (job->transforms_applied) return;

    int count = job->parse_state.parsed_count;
    vec3_t *points = job->parse_state.points;

    // Step 1: Calculate and apply Centre of Mass shift
    if (job->shift_to_com && count > 0) {
        // Calculate CoM
        vec3_t sum = vec3_make(0, 0, 0);
        for (int i = 0; i < count; i++) {
            sum = vec3_add(sum, points[i]);
        }
        job->com = vec3_scale(sum, 1.0f / (float)count);

        // Shift all points by -CoM
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
        // Combined: Z * Y * X (applied right to left)
        mat4_t rot_xy = mat4_mul(rot_y, rot_x);
        job->transform_matrix = mat4_mul(rot_z, rot_xy);

        // Apply rotation to all points
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
        // Start timing this iteration
        uint64_t iter_start = stm_now();

        int parsed = ply_parse_vertices_chunk(&job->parse_state, PLY_PARSE_CHUNK_SIZE);
        (void)parsed;  // Suppress unused warning

        // Record iteration timing
        uint64_t iter_end = stm_now();
        job->last_iteration_time_ms = stm_ms(stm_diff(iter_end, iter_start));

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

        // Store timing sample for the plot (sample every ~1% progress to ensure even distribution)
        float progress_pct = job->progress * 100.0f;
        if (job->timing_sample_count < PLY_JOB_MAX_TIMING_SAMPLES &&
            (progress_pct - job->last_sampled_progress) >= 1.0f) {
            job->iteration_times[job->timing_sample_count] = (float)job->last_iteration_time_ms;
            job->iteration_progress[job->timing_sample_count] = progress_pct;
            job->timing_sample_count++;
            job->last_sampled_progress = progress_pct;
        }

        snprintf(job->status_message, sizeof(job->status_message),
                 "Parsing: %d / %d vertices",
                 job->parse_state.parsed_count, job->total_points);

        // Check if parsing is complete
        if (ply_is_complete(&job->parse_state)) {
            ply_close(&job->parse_state);  // Close file, keep data

            // Apply all transformations to parsed data (CoM shift, rotation, scale)
            ply_import_job_apply_transforms(job);

            if (job->import_mode == 0) {
                // Point Cloud Node mode - create single entity
                // Transformations already applied to coordinates, so use identity transform
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
                (void)cloud;  // Entity created, no further setup needed

                // Free parsed data
                ply_parse_state_free(&job->parse_state);

                job->progress = 1.0f;
                job->state = PLY_JOB_COMPLETE;

                // Record final 100% sample for the plot
                if (job->timing_sample_count < PLY_JOB_MAX_TIMING_SAMPLES) {
                    job->iteration_times[job->timing_sample_count] = (float)job->last_iteration_time_ms;
                    job->iteration_progress[job->timing_sample_count] = 100.0f;
                    job->timing_sample_count++;
                }

                snprintf(job->status_message, sizeof(job->status_message),
                         "Imported %d points as Point Cloud", job->total_points);
                return true;
            } else {
                // Editable Subtree mode - create individual point entities (no parent)
                // Transformations already applied to coordinates
                job->created_count = 0;
                job->state = PLY_JOB_CREATING_ENTITIES;
                snprintf(job->status_message, sizeof(job->status_message),
                         "Creating: 0 / %d entities", job->total_points);
            }
        }

        return false;  // Not complete yet
    }

    //--------------------------------------------------------------------------
    // State: CREATING_ENTITIES (Editable mode)
    // Creates entities as top-level (no parent) with pre-transformed coordinates
    //--------------------------------------------------------------------------
    if (job->state == PLY_JOB_CREATING_ENTITIES) {
        // Start timing this iteration
        uint64_t iter_start = stm_now();

        int remaining = job->total_points - job->created_count;
        int to_create = (remaining < PLY_ENTITY_CHUNK_SIZE) ? remaining : PLY_ENTITY_CHUNK_SIZE;

        vec4_t *colors = NULL;
        if (job->use_ply_colors && job->parse_state.has_colors) {
            colors = job->parse_state.colors;
        }

        // Create entities as top-level (no parenting)
        // Coordinates already have CoM shift, rotation, and scale applied
        for (int i = 0; i < to_create; i++) {
            int idx = job->created_count + i;
            vec4_t pt_color = colors ? colors[idx] : job->default_color;
            scene_add_point(
                scene,
                job->parse_state.points[idx],
                pt_color,
                job->point_size
            );
        }

        job->created_count += to_create;

        // Record iteration timing
        uint64_t iter_end = stm_now();
        job->last_iteration_time_ms = stm_ms(stm_diff(iter_end, iter_start));

        // Update progress - entity creation is 30-100% (70% of total)
        float entity_progress = (float)job->created_count / (float)job->total_points;
        job->progress = 0.30f + 0.70f * entity_progress;

        // Store timing sample for the plot (sample every ~1% progress to ensure even distribution)
        float progress_pct = job->progress * 100.0f;
        if (job->timing_sample_count < PLY_JOB_MAX_TIMING_SAMPLES &&
            (progress_pct - job->last_sampled_progress) >= 1.0f) {
            job->iteration_times[job->timing_sample_count] = (float)job->last_iteration_time_ms;
            job->iteration_progress[job->timing_sample_count] = progress_pct;
            job->timing_sample_count++;
            job->last_sampled_progress = progress_pct;
        }

        snprintf(job->status_message, sizeof(job->status_message),
                 "Creating: %d / %d entities",
                 job->created_count, job->total_points);

        // Check if creation is complete
        if (job->created_count >= job->total_points) {
            // Free parsed data (no longer needed)
            ply_parse_state_free(&job->parse_state);

            job->progress = 1.0f;
            job->state = PLY_JOB_COMPLETE;

            // Record final 100% sample for the plot
            if (job->timing_sample_count < PLY_JOB_MAX_TIMING_SAMPLES) {
                job->iteration_times[job->timing_sample_count] = (float)job->last_iteration_time_ms;
                job->iteration_progress[job->timing_sample_count] = 100.0f;
                job->timing_sample_count++;
            }

            snprintf(job->status_message, sizeof(job->status_message),
                     "Imported %d points as Editable Points", job->total_points);
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
    (void)scene;  // No longer need to delete entities - they're top-level and stay

    if (job->state == PLY_JOB_IDLE ||
        job->state == PLY_JOB_COMPLETE ||
        job->state == PLY_JOB_CANCELLED ||
        job->state == PLY_JOB_ERROR) {
        return;  // Nothing to cancel
    }

    // Free parsed data
    ply_parse_state_free(&job->parse_state);

    // Note: For Editable mode, created entities are top-level (no parent).
    // On cancel, we leave them in the scene rather than trying to delete them,
    // as tracking which ones were created adds complexity.
    // Users can clear the scene if needed.

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
    job->transforms_applied = false;
}

#endif // PLY_IMPORT_JOB_H
