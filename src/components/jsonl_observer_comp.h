//------------------------------------------------------------------------------
// jsonl_observer_comp.h - Linked JSONL observer metadata component
//------------------------------------------------------------------------------
#ifndef JSONL_OBSERVER_COMP_H
#define JSONL_OBSERVER_COMP_H

#include "component_types.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define JSONL_OBSERVER_PATH_MAX 1024
#define JSONL_OBSERVER_MESSAGE_MAX 192
#define JSONL_OBSERVER_MESSAGE_HISTORY 2
#define JSONL_OBSERVER_DEFAULT_INTERVAL_MS 1000u
#define JSONL_OBSERVER_DEFAULT_MAX_RETRIES 50u

typedef enum {
    JSONL_OBSERVER_MSG_INFO = 0,
    JSONL_OBSERVER_MSG_WARNING = 1,
    JSONL_OBSERVER_MSG_ERROR = 2
} jsonl_observer_msg_severity_t;

typedef struct {
    bool linked;
    bool observe_enabled;
    uint32_t interval_ms;
    uint32_t max_retries;
    uint32_t retry_count;
    uint64_t next_retry_at_ms;

    float scale;
    float rotation_x;
    float rotation_y;
    float rotation_z;
    bool shift_to_center;

    char source_path[JSONL_OBSERVER_PATH_MAX];

    uint32_t message_count;
    uint8_t message_severity[JSONL_OBSERVER_MESSAGE_HISTORY];
    char messages[JSONL_OBSERVER_MESSAGE_HISTORY][JSONL_OBSERVER_MESSAGE_MAX];
} JsonlObserverComp;

static inline JsonlObserverComp jsonl_observer_comp_default(void) {
    JsonlObserverComp c;
    memset(&c, 0, sizeof(c));
    c.observe_enabled = true;
    c.interval_ms = JSONL_OBSERVER_DEFAULT_INTERVAL_MS;
    c.max_retries = JSONL_OBSERVER_DEFAULT_MAX_RETRIES;
    c.scale = 1.0f;
    c.rotation_x = 0.0f;
    c.rotation_y = 0.0f;
    c.rotation_z = 0.0f;
    c.shift_to_center = false;
    return c;
}

static inline void jsonl_observer_comp_set_path(JsonlObserverComp *comp, const char *path) {
    if (!comp) return;
    comp->linked = (path && path[0] != '\0');
    if (!path) path = "";
    strncpy(comp->source_path, path, JSONL_OBSERVER_PATH_MAX - 1);
    comp->source_path[JSONL_OBSERVER_PATH_MAX - 1] = '\0';
}

static inline void jsonl_observer_comp_push_message(JsonlObserverComp *comp,
                                                    jsonl_observer_msg_severity_t severity,
                                                    const char *message) {
    if (!comp) return;
    if (!message) message = "";

    if (comp->message_count < JSONL_OBSERVER_MESSAGE_HISTORY) {
        uint32_t idx = comp->message_count++;
        comp->message_severity[idx] = (uint8_t)severity;
        strncpy(comp->messages[idx], message, JSONL_OBSERVER_MESSAGE_MAX - 1);
        comp->messages[idx][JSONL_OBSERVER_MESSAGE_MAX - 1] = '\0';
        return;
    }

    // Keep latest two events: shift [1] -> [0], write newest at [1]
    comp->message_severity[0] = comp->message_severity[1];
    strncpy(comp->messages[0], comp->messages[1], JSONL_OBSERVER_MESSAGE_MAX - 1);
    comp->messages[0][JSONL_OBSERVER_MESSAGE_MAX - 1] = '\0';

    comp->message_severity[1] = (uint8_t)severity;
    strncpy(comp->messages[1], message, JSONL_OBSERVER_MESSAGE_MAX - 1);
    comp->messages[1][JSONL_OBSERVER_MESSAGE_MAX - 1] = '\0';
}

#endif // JSONL_OBSERVER_COMP_H

