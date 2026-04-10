//------------------------------------------------------------------------------
// sketch_script_apply.h - Two-pass script apply helpers
//------------------------------------------------------------------------------
#ifndef SKETCH_SCRIPT_APPLY_H
#define SKETCH_SCRIPT_APPLY_H

#include "sketch_script_parse.h"
#include "../components/script_identity_comp.h"
// Requires ecs_scene.h to be included before this header.

typedef struct {
    char id[SCRIPT_LOCAL_ID_MAX];
    ecs_entity_t entity;
} sketch_script_apply_link_t;

typedef struct {
    uint32_t count;
    sketch_script_apply_link_t items[SKETCH_SCRIPT_MODEL_MAX_ENTITIES + SKETCH_SCRIPT_MODEL_MAX_CONSTRAINTS];
} sketch_script_apply_link_table_t;

typedef struct {
    bool has_pair;
    bool pair_is_owner;
    char owner_id[SCRIPT_LOCAL_ID_MAX];
    char paired_id[SCRIPT_LOCAL_ID_MAX];
} sketch_script_apply_pair_meta_t;

typedef struct {
    char id[SCRIPT_LOCAL_ID_MAX];
    sketch_script_apply_pair_meta_t meta;
} sketch_script_apply_pair_entry_t;

typedef struct {
    uint32_t count;
    sketch_script_apply_pair_entry_t items[SKETCH_SCRIPT_MODEL_MAX_CONSTRAINTS];
} sketch_script_apply_pair_table_t;

typedef struct {
    char id[SCRIPT_LOCAL_ID_MAX];
    LabelComp label;
} sketch_script_apply_label_entry_t;

typedef struct {
    uint32_t count;
    sketch_script_apply_label_entry_t items[SKETCH_SCRIPT_MODEL_MAX_ENTITIES + SKETCH_SCRIPT_MODEL_MAX_CONSTRAINTS];
} sketch_script_apply_label_table_t;

static inline void sketch_script_apply_set_error(sketch_script_error_t *out_error,
                                                 const char *message) {
    if (!out_error) return;
    out_error->line = 0;
    out_error->column = 0;
    snprintf(out_error->message, sizeof(out_error->message), "%s", message ? message : "Apply error.");
    out_error->message[sizeof(out_error->message) - 1] = '\0';
}

static inline bool sketch_script_apply_link_add(sketch_script_apply_link_table_t *table,
                                                const char *id,
                                                ecs_entity_t entity) {
    if (!table || !id || id[0] == '\0' || entity == 0) return false;
    if (table->count >= (uint32_t)(sizeof(table->items) / sizeof(table->items[0]))) return false;
    strncpy(table->items[table->count].id, id, SCRIPT_LOCAL_ID_MAX - 1);
    table->items[table->count].id[SCRIPT_LOCAL_ID_MAX - 1] = '\0';
    table->items[table->count].entity = entity;
    table->count++;
    return true;
}

static inline ecs_entity_t sketch_script_apply_link_find(const sketch_script_apply_link_table_t *table,
                                                         const char *id) {
    if (!table || !id || id[0] == '\0') return 0;
    for (uint32_t i = 0; i < table->count; i++) {
        if (strcmp(table->items[i].id, id) == 0) return table->items[i].entity;
    }
    return 0;
}

static inline bool sketch_script_apply_label_add(sketch_script_apply_label_table_t *table,
                                                 const char *id,
                                                 const LabelComp *label) {
    if (!table || !id || id[0] == '\0' || !label) return false;
    if (table->count >= (uint32_t)(sizeof(table->items) / sizeof(table->items[0]))) return false;
    for (uint32_t i = 0; i < table->count; i++) {
        if (strcmp(table->items[i].id, id) == 0) {
            table->items[i].label = *label;
            return true;
        }
    }
    strncpy(table->items[table->count].id, id, SCRIPT_LOCAL_ID_MAX - 1);
    table->items[table->count].id[SCRIPT_LOCAL_ID_MAX - 1] = '\0';
    table->items[table->count].label = *label;
    table->count++;
    return true;
}

static inline bool sketch_script_apply_label_find(const sketch_script_apply_label_table_t *table,
                                                  const char *id,
                                                  LabelComp *out_label) {
    if (!table || !id || id[0] == '\0' || !out_label) return false;
    for (uint32_t i = 0; i < table->count; i++) {
        if (strcmp(table->items[i].id, id) == 0) {
            *out_label = table->items[i].label;
            return true;
        }
    }
    return false;
}

static inline bool sketch_script_apply_pair_add(sketch_script_apply_pair_table_t *table,
                                                const char *id,
                                                const sketch_script_apply_pair_meta_t *meta) {
    if (!table || !id || id[0] == '\0' || !meta) return false;
    if (!meta->has_pair) return true;
    if (table->count >= (uint32_t)(sizeof(table->items) / sizeof(table->items[0]))) return false;
    strncpy(table->items[table->count].id, id, SCRIPT_LOCAL_ID_MAX - 1);
    table->items[table->count].id[SCRIPT_LOCAL_ID_MAX - 1] = '\0';
    table->items[table->count].meta = *meta;
    table->count++;
    return true;
}

static inline bool sketch_script_apply_pair_find(const sketch_script_apply_pair_table_t *table,
                                                 const char *id,
                                                 sketch_script_apply_pair_meta_t *out_meta) {
    if (!table || !id || id[0] == '\0' || !out_meta) return false;
    for (uint32_t i = 0; i < table->count; i++) {
        if (strcmp(table->items[i].id, id) == 0) {
            *out_meta = table->items[i].meta;
            return true;
        }
    }
    return false;
}

static inline void sketch_script_apply_snapshot_labels(ecs_scene_t *scene,
                                                       ecs_entity_t sketch,
                                                       sketch_script_apply_label_table_t *out_labels) {
    if (!scene || sketch == 0 || !out_labels) return;
    out_labels->count = 0;
    ecs_iter_t it = ecs_children(scene->world->world, sketch);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            ecs_entity_t child = it.entities[i];
            if (!ecs_world_get_geometry(scene->world, child) &&
                !ecs_world_get_constraint(scene->world, child)) {
                continue;
            }
            ScriptIdentityComp *sid = ecs_world_get_script_identity(scene->world, child);
            LabelComp *label = ecs_world_get_label(scene->world, child);
            if (!sid || sid->script_local_id[0] == '\0' || !label || label->name[0] == '\0') {
                continue;
            }
            (void)sketch_script_apply_label_add(out_labels, sid->script_local_id, label);
        }
    }
}

static inline void sketch_script_apply_add_script_identity(ecs_scene_t *scene,
                                                           ecs_entity_t entity,
                                                           const char *id) {
    if (!scene || entity == 0 || !id) return;
    ScriptIdentityComp sid = script_identity_comp_make(id);
    ecs_world_set_script_identity(scene->world, entity, &sid);
}

static inline bool sketch_script_apply_create_entities(ecs_scene_t *scene,
                                                       ecs_entity_t sketch,
                                                       const sketch_script_model_t *model,
                                                       sketch_script_apply_link_table_t *links,
                                                       const sketch_script_apply_label_table_t *labels,
                                                       sketch_script_error_t *out_error) {
    for (uint32_t i = 0; i < model->entity_count; i++) {
        const sketch_script_entity_model_t *src = &model->entities[i];
        ecs_entity_t entity = 0;
        if (src->kind == SKETCH_SCRIPT_ENTITY_POINT) {
            entity = scene_add_point_to_sketch(scene, sketch, src->point, src->color, 0.01f);
        } else if (src->kind == SKETCH_SCRIPT_ENTITY_LINE) {
            entity = scene_add_line_to_sketch(scene, sketch, src->a, src->b, src->color, 1.0f);
        } else {
            entity = scene_add_arc_to_sketch(scene, sketch,
                                             src->center, src->radius,
                                             src->start_angle, src->end_angle,
                                             src->normal, src->color, 1.0f);
        }
        if (entity == 0) {
            sketch_script_apply_set_error(out_error, "Failed creating script entity.");
            return false;
        }
        sketch_script_apply_add_script_identity(scene, entity, src->id);
        LabelComp restored = {0};
        if (labels && sketch_script_apply_label_find(labels, src->id, &restored)) {
            ecs_world_set_label(scene->world, entity, &restored);
        }
        if (!sketch_script_apply_link_add(links, src->id, entity)) {
            sketch_script_apply_set_error(out_error, "Failed linking script entity ID.");
            return false;
        }
    }
    return true;
}

static inline bool sketch_script_apply_create_constraints(ecs_scene_t *scene,
                                                          ecs_entity_t sketch,
                                                          const sketch_script_model_t *model,
                                                          sketch_script_apply_link_table_t *links,
                                                          sketch_script_apply_pair_table_t *pairs,
                                                          const sketch_script_apply_label_table_t *labels,
                                                          sketch_script_error_t *out_error) {
    for (uint32_t i = 0; i < model->constraint_count; i++) {
        const sketch_script_constraint_model_t *src = &model->constraints[i];
        constraint_participant_descriptor_t participants[CONSTRAINT_MAX_PARTICIPANTS] = {0};
        for (uint32_t p = 0; p < src->participant_count; p++) {
            const sketch_script_participant_model_t *participant = &src->participants[p];
            ecs_entity_t participant_entity = sketch_script_apply_link_find(links, participant->id);
            if (participant_entity == 0) {
                sketch_script_apply_set_error(out_error, "Unresolved participant reference.");
                return false;
            }
            GeometryComp *participant_geom = ecs_world_get_geometry(scene->world, participant_entity);
            if (!participant_geom) {
                sketch_script_apply_set_error(out_error, "Participant has no geometry.");
                return false;
            }
            if (!sketch_script_capability_role_allowed_for_geometry(participant_geom->type, participant->role)) {
                sketch_script_apply_set_error(out_error, "Participant descriptor role is not supported for geometry.");
                return false;
            }
            participants[p] = constraint_participant_descriptor_make((uint64_t)participant_entity,
                                                                     (uint8_t)participant->role,
                                                                     participant->sub_index);
        }

        ecs_entity_t constraint = scene_add_constraint_to_sketch_with_descriptors(scene, sketch, src->type,
                                                                                   participants, src->participant_count,
                                                                                   src->has_value ? src->value : 0.0f,
                                                                                   src->driven);
        if (constraint == 0) {
            sketch_script_apply_set_error(out_error, "Failed creating script constraint.");
            return false;
        }
        sketch_script_apply_add_script_identity(scene, constraint, src->id);
        LabelComp restored = {0};
        if (labels && sketch_script_apply_label_find(labels, src->id, &restored)) {
            ecs_world_set_label(scene->world, constraint, &restored);
        }
        if (!sketch_script_apply_link_add(links, src->id, constraint)) {
            sketch_script_apply_set_error(out_error, "Failed linking script constraint ID.");
            return false;
        }
        sketch_script_apply_pair_meta_t pair_meta = {0};
        pair_meta.has_pair = src->has_pair;
        pair_meta.pair_is_owner = src->pair_is_owner;
        if (src->has_pair) {
            strncpy(pair_meta.owner_id, src->pair_owner_id, SCRIPT_LOCAL_ID_MAX - 1);
            pair_meta.owner_id[SCRIPT_LOCAL_ID_MAX - 1] = '\0';
            strncpy(pair_meta.paired_id, src->pair_paired_id, SCRIPT_LOCAL_ID_MAX - 1);
            pair_meta.paired_id[SCRIPT_LOCAL_ID_MAX - 1] = '\0';
        if (!sketch_script_apply_pair_add(pairs, src->id, &pair_meta)) {
            sketch_script_apply_set_error(out_error, "Failed storing script constraint pair metadata.");
            return false;
        }
        }
    }
    return true;
}

static inline bool sketch_script_apply_restore_pairs(ecs_scene_t *scene,
                                                     const sketch_script_apply_pair_table_t *pairs,
                                                     const sketch_script_apply_link_table_t *links,
                                                     sketch_script_error_t *out_error) {
    if (!scene || !pairs || !links) {
        sketch_script_apply_set_error(out_error, "Invalid pair metadata apply state.");
        return false;
    }
    for (uint32_t i = 0; i < pairs->count; i++) {
        const sketch_script_apply_pair_entry_t *entry = &pairs->items[i];
        ecs_entity_t self = sketch_script_apply_link_find(links, entry->id);
        ecs_entity_t owner = sketch_script_apply_link_find(links, entry->meta.owner_id);
        ecs_entity_t paired = sketch_script_apply_link_find(links, entry->meta.paired_id);
        if (self == 0 || owner == 0 || paired == 0) {
            sketch_script_apply_set_error(out_error, "Unresolved pair metadata constraint reference.");
            return false;
        }
        ConstraintComp *self_constraint = ecs_world_get_constraint(scene->world, self);
        ConstraintComp *owner_constraint = ecs_world_get_constraint(scene->world, owner);
        ConstraintComp *paired_constraint = ecs_world_get_constraint(scene->world, paired);
        if (!self_constraint || !owner_constraint || !paired_constraint) {
            sketch_script_apply_set_error(out_error, "Pair metadata references non-constraint entities.");
            return false;
        }
        self_constraint->pair_is_owner = entry->meta.pair_is_owner;
        self_constraint->pair_owner_constraint_entity = (uint64_t)owner;
        self_constraint->paired_constraint_entity = (uint64_t)paired;
    }
    return true;
}

static inline int sketch_script_snapshot_sketch_children(ecs_scene_t *scene,
                                                         ecs_entity_t sketch,
                                                         ecs_entity_t *out_children,
                                                         int max_children) {
    if (!scene || sketch == 0 || !out_children || max_children <= 0) return 0;
    int count = 0;
    ecs_iter_t it = ecs_children(scene->world->world, sketch);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count && count < max_children; i++) {
            out_children[count++] = it.entities[i];
        }
        if (count >= max_children) break;
    }
    return count;
}

static inline int sketch_script_snapshot_script_children(ecs_scene_t *scene,
                                                         ecs_entity_t sketch,
                                                         ecs_entity_t *out_children,
                                                         int max_children) {
    if (!scene || sketch == 0 || !out_children || max_children <= 0) return 0;
    int count = 0;
    ecs_iter_t it = ecs_children(scene->world->world, sketch);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count && count < max_children; i++) {
            ecs_entity_t child = it.entities[i];
            ScriptIdentityComp *sid = ecs_world_get_script_identity(scene->world, child);
            if (!sid || sid->script_local_id[0] == '\0') continue;
            out_children[count++] = child;
        }
        if (count >= max_children) break;
    }
    return count;
}

static inline void sketch_script_remove_children(ecs_scene_t *scene,
                                                 ecs_entity_t *children,
                                                 int child_count) {
    if (!scene || !children || child_count <= 0) return;
    for (int i = 0; i < child_count; i++) {
        if (children[i] != 0 && ecs_is_alive(scene->world->world, children[i])) {
            scene_remove_entity(scene, children[i]);
        }
    }
}

static inline bool sketch_script_apply_model_on_sketch(ecs_scene_t *scene,
                                                        ecs_entity_t sketch,
                                                        const sketch_script_model_t *model,
                                                        const sketch_script_apply_label_table_t *labels,
                                                        sketch_script_error_t *out_error) {
    if (!scene || !scene_is_sketch(scene, sketch) || !model) {
        sketch_script_apply_set_error(out_error, "Invalid scene/sketch/model.");
        return false;
    }

    sketch_script_apply_link_table_t links = {0};
    sketch_script_apply_pair_table_t pairs = {0};
    if (!sketch_script_apply_create_entities(scene, sketch, model, &links, labels, out_error)) return false;
    if (!sketch_script_apply_create_constraints(scene, sketch, model, &links, &pairs, labels, out_error)) return false;
    if (!sketch_script_apply_restore_pairs(scene, &pairs, &links, out_error)) return false;
    scene_refresh_sketch_metadata(scene, sketch);
    return true;
}

static inline bool sketch_script_apply_validate_model_links(const sketch_script_model_t *model,
                                                            sketch_script_error_t *out_error) {
    if (!model) return false;
    for (uint32_t i = 0; i < model->constraint_count; i++) {
        const sketch_script_constraint_model_t *constraint = &model->constraints[i];
        constraint_selection_signature_t sig = {0};
        sig.count = constraint->participant_count;
        for (uint32_t p = 0; p < constraint->participant_count; p++) {
            bool found = false;
            for (uint32_t e = 0; e < model->entity_count; e++) {
                if (strcmp(model->entities[e].id, constraint->participants[p].id) != 0) continue;
                found = true;
                sig.entities[p] = (uint64_t)(e + 1u);
                if (model->entities[e].kind == SKETCH_SCRIPT_ENTITY_POINT) {
                    sig.geometry_types[p] = GEOM_POINT;
                } else if (model->entities[e].kind == SKETCH_SCRIPT_ENTITY_LINE) {
                    sig.geometry_types[p] = GEOM_LINE;
                } else {
                    sig.geometry_types[p] = GEOM_ARC;
                }
                sig.roles[p] = constraint->participants[p].role;
                if (!sketch_script_capability_role_allowed_for_geometry(sig.geometry_types[p], sig.roles[p])) {
                    sketch_script_apply_set_error(out_error, "Participant descriptor role is not supported for geometry.");
                    return false;
                }
                break;
            }
            if (!found) {
                sketch_script_apply_set_error(out_error, "Unresolved participant reference.");
                return false;
            }
        }
        char capability_error[192] = {0};
        if (!sketch_script_capability_validate_signature(&sig, constraint->type,
                                                         capability_error, sizeof(capability_error))) {
            sketch_script_apply_set_error(out_error, capability_error);
            return false;
        }
    }
    return true;
}

static inline bool sketch_script_apply_preview_model(ecs_scene_t *scene,
                                                     ecs_entity_t sketch,
                                                     const char *script_text,
                                                     sketch_script_error_t *out_error) {
    sketch_script_model_t model = {0};
    if (!sketch_script_parse_model(script_text, &model, out_error)) return false;
    if (!scene_is_sketch(scene, sketch)) {
        sketch_script_apply_set_error(out_error, "Invalid scene/sketch/script.");
        return false;
    }
    return sketch_script_apply_validate_model_links(&model, out_error);
}

static inline bool sketch_script_apply_commit_model(ecs_scene_t *scene,
                                                    ecs_entity_t sketch,
                                                    const char *script_text,
                                                    sketch_script_error_t *out_error) {
    if (!scene || !scene_is_sketch(scene, sketch) || !script_text) {
        sketch_script_apply_set_error(out_error, "Invalid scene/sketch/script.");
        return false;
    }
    sketch_script_model_t model = {0};
    if (!sketch_script_parse_model(script_text, &model, out_error)) return false;
    if (!sketch_script_apply_validate_model_links(&model, out_error)) return false;

    sketch_script_apply_label_table_t previous_labels = {0};
    sketch_script_apply_snapshot_labels(scene, sketch, &previous_labels);

    ecs_entity_t previous_children[1024] = {0};
    int previous_count = sketch_script_snapshot_script_children(scene, sketch, previous_children, 1024);
    sketch_script_remove_children(scene, previous_children, previous_count);

    if (!sketch_script_apply_model_on_sketch(scene, sketch, &model, &previous_labels, out_error)) {
        ecs_entity_t created_after[1024] = {0};
        int created_count = sketch_script_snapshot_script_children(scene, sketch, created_after, 1024);
        sketch_script_remove_children(scene, created_after, created_count);
        return false;
    }
    return true;
}

#endif // SKETCH_SCRIPT_APPLY_H
