#ifndef STARTUP_JSONL_IMPORT_CONTROLLER_H
#define STARTUP_JSONL_IMPORT_CONTROLLER_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "app_launch_config.h"
#include "jsonl_import_job.h"

typedef enum {
    STARTUP_JSONL_IMPORT_IDLE = 0,
    STARTUP_JSONL_IMPORT_ARMED,
    STARTUP_JSONL_IMPORT_RUNNING,
    STARTUP_JSONL_IMPORT_COMPLETE,
    STARTUP_JSONL_IMPORT_ERROR
} startup_jsonl_import_state_t;

typedef struct {
    startup_jsonl_import_state_t state;
    jsonl_import_job_t job;
    char requested_path[MDCAD_STARTUP_JSONL_PATH_MAX];
    char status_text[128];
    char error_text[128];
    bool scene_dirty;
} startup_jsonl_import_controller_t;

static inline void startup_jsonl_import_controller_init(startup_jsonl_import_controller_t *controller) {
    if (!controller) {
        return;
    }
    memset(controller, 0, sizeof(*controller));
    jsonl_import_job_init(&controller->job);
    controller->state = STARTUP_JSONL_IMPORT_IDLE;
}

static inline bool startup_jsonl_import_controller_arm(startup_jsonl_import_controller_t *controller,
                                                       const char *path) {
    if (!controller || !path || path[0] == '\0') {
        return false;
    }
    snprintf(controller->requested_path, sizeof(controller->requested_path), "%s", path);
    controller->state = STARTUP_JSONL_IMPORT_ARMED;
    snprintf(controller->status_text, sizeof(controller->status_text), "Pending startup import");
    controller->error_text[0] = '\0';
    controller->scene_dirty = false;
    return true;
}

static inline bool startup_jsonl_import_controller_tick(startup_jsonl_import_controller_t *controller,
                                                        ecs_scene_t *scene) {
    (void)scene;
    if (!controller) {
        return false;
    }
    controller->scene_dirty = false;
    return false;
}

static inline void startup_jsonl_import_controller_reset(startup_jsonl_import_controller_t *controller,
                                                         ecs_scene_t *scene) {
    if (!controller) {
        return;
    }
    if (jsonl_import_job_is_running(&controller->job)) {
        jsonl_import_job_cancel(&controller->job, scene);
    } else {
        jsonl_import_job_reset(&controller->job);
    }
    controller->state = STARTUP_JSONL_IMPORT_IDLE;
    controller->requested_path[0] = '\0';
    controller->status_text[0] = '\0';
    controller->error_text[0] = '\0';
    controller->scene_dirty = false;
}

static inline void startup_jsonl_import_controller_dismiss_error(startup_jsonl_import_controller_t *controller) {
    if (!controller) {
        return;
    }
    controller->error_text[0] = '\0';
    if (controller->state == STARTUP_JSONL_IMPORT_ERROR) {
        controller->state = STARTUP_JSONL_IMPORT_IDLE;
    }
}

#endif // STARTUP_JSONL_IMPORT_CONTROLLER_H
