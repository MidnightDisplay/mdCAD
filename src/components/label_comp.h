//------------------------------------------------------------------------------
// label_comp.h - Label component for ECS entities (header-only)
//
// Stores a human-readable name and description for entities.
// Populated during JSONL import from the Name/Description fields.
//------------------------------------------------------------------------------
#ifndef LABEL_COMP_H
#define LABEL_COMP_H

#include <string.h>

#define LABEL_NAME_MAX 128
#define LABEL_DESC_MAX 256

typedef struct {
    char name[LABEL_NAME_MAX];
    char description[LABEL_DESC_MAX];
} LabelComp;

static inline LabelComp label_comp_default(void) {
    LabelComp l;
    memset(&l, 0, sizeof(LabelComp));
    return l;
}

static inline LabelComp label_comp_make(const char *name, const char *description) {
    LabelComp l;
    memset(&l, 0, sizeof(LabelComp));
    if (name) {
        strncpy(l.name, name, LABEL_NAME_MAX - 1);
        l.name[LABEL_NAME_MAX - 1] = '\0';
    }
    if (description) {
        strncpy(l.description, description, LABEL_DESC_MAX - 1);
        l.description[LABEL_DESC_MAX - 1] = '\0';
    }
    return l;
}

#endif // LABEL_COMP_H
