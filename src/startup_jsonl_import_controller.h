#ifndef STARTUP_JSONL_IMPORT_CONTROLLER_H
#define STARTUP_JSONL_IMPORT_CONTROLLER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
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
    bool live_refresh_enabled;
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

static inline void startup_jsonl_import_controller_forget_deleted_root(startup_jsonl_import_controller_t *controller,
                                                                       ecs_scene_t *scene) {
    if (!controller || !scene || !scene->world) {
        return;
    }
    if (controller->state != STARTUP_JSONL_IMPORT_COMPLETE) {
        return;
    }

    ecs_entity_t root_entity = controller->job.root_entity;
    if (root_entity != 0 && !ecs_is_alive(scene->world->world, root_entity)) {
        controller->job.root_entity = 0;
        controller->status_text[0] = '\0';
    }
}

static inline bool startup_jsonl_import_controller_arm(startup_jsonl_import_controller_t *controller,
                                                       const char *path,
                                                       bool live_refresh_enabled) {
    if (!controller || !path || path[0] == '\0') {
        return false;
    }
    controller->live_refresh_enabled = live_refresh_enabled;
    snprintf(controller->requested_path, sizeof(controller->requested_path), "%s", path);
    controller->state = STARTUP_JSONL_IMPORT_ARMED;
    snprintf(controller->status_text, sizeof(controller->status_text), "Pending startup import");
    controller->error_text[0] = '\0';
    controller->scene_dirty = false;
    return true;
}

static inline bool startup_jsonl_import_controller_tick(startup_jsonl_import_controller_t *controller,
                                                        ecs_scene_t *scene) {
    if (!controller) {
        return false;
    }
    controller->scene_dirty = false;
    if (scene) {
        startup_jsonl_import_controller_forget_deleted_root(controller, scene);
    }
    if (controller->state == STARTUP_JSONL_IMPORT_IDLE ||
        controller->state == STARTUP_JSONL_IMPORT_COMPLETE ||
        controller->state == STARTUP_JSONL_IMPORT_ERROR) {
        return false;
    }

    if (controller->state == STARTUP_JSONL_IMPORT_ARMED) {
        bool started = jsonl_import_job_start(&controller->job,
                                              controller->requested_path,
                                              1.0f,
                                              true,
                                              vec4_make(1.0f, 1.0f, 1.0f, 1.0f),
                                              false,
                                              0.0f, 0.0f, 0.0f);
        if (!started) {
            controller->state = STARTUP_JSONL_IMPORT_ERROR;
            snprintf(controller->error_text, sizeof(controller->error_text), "%s", controller->job.status_message);
            controller->status_text[0] = '\0';
            return false;
        }
        jsonl_import_job_set_mesh_mode(&controller->job, 0);
        jsonl_import_job_set_observer_contract(&controller->job,
                                               controller->live_refresh_enabled,
                                               controller->requested_path);
        controller->state = STARTUP_JSONL_IMPORT_RUNNING;
        snprintf(controller->status_text, sizeof(controller->status_text), "%s", controller->job.status_message);
        if (jsonl_import_job_should_sync(&controller->job)) {
            while (!jsonl_import_job_tick(&controller->job, scene)) {
            }
        }
    }

    if (controller->state == STARTUP_JSONL_IMPORT_RUNNING) {
        if (!jsonl_import_job_should_sync(&controller->job)) {
            bool finished = jsonl_import_job_tick(&controller->job, scene);
            if (!finished) {
                snprintf(controller->status_text, sizeof(controller->status_text), "%s", controller->job.status_message);
                return false;
            }
        }

        if (controller->job.state == JSONL_JOB_COMPLETE) {
            controller->state = STARTUP_JSONL_IMPORT_COMPLETE;
            controller->scene_dirty = true;
            snprintf(controller->status_text, sizeof(controller->status_text), "%s", controller->job.status_message);
            controller->error_text[0] = '\0';
            return true;
        }
        if (controller->job.state == JSONL_JOB_ERROR) {
            controller->state = STARTUP_JSONL_IMPORT_ERROR;
            controller->status_text[0] = '\0';
            snprintf(controller->error_text, sizeof(controller->error_text), "%s", controller->job.status_message);
            return false;
        }
    }

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
    controller->live_refresh_enabled = false;
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
    controller->status_text[0] = '\0';
}

#endif // STARTUP_JSONL_IMPORT_CONTROLLER_H
