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
                                                       sketch_script_error_t *out_error) {
    for (uint32_t i = 0; i < model->entity_count; i++) {
        const sketch_script_entity_model_t *src = &model->entities[i];
        ecs_entity_t entity = 0;
        if (src->kind == SKETCH_SCRIPT_ENTITY_POINT) {
            entity = scene_add_point_to_sketch(scene, sketch, src->point, vec4_make(1, 1, 1, 1), 0.01f);
        } else if (src->kind == SKETCH_SCRIPT_ENTITY_LINE) {
            entity = scene_add_line_to_sketch(scene, sketch, src->a, src->b, vec4_make(1, 1, 1, 1), 1.0f);
        } else {
            entity = scene_add_arc_to_sketch(scene, sketch,
                                             src->center, src->radius,
                                             src->start_angle, src->end_angle,
                                             src->normal, vec4_make(1, 1, 1, 1), 1.0f);
        }
        if (entity == 0) {
            sketch_script_apply_set_error(out_error, "Failed creating script entity.");
            return false;
        }
        sketch_script_apply_add_script_identity(scene, entity, src->id);
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
                                                          sketch_script_error_t *out_error) {
    for (uint32_t i = 0; i < model->constraint_count; i++) {
        const sketch_script_constraint_model_t *src = &model->constraints[i];
        ecs_entity_t participants[CONSTRAINT_MAX_PARTICIPANTS] = {0};
        for (uint32_t p = 0; p < src->participant_count; p++) {
            participants[p] = sketch_script_apply_link_find(links, src->participants[p]);
            if (participants[p] == 0) {
                sketch_script_apply_set_error(out_error, "Unresolved participant reference.");
                return false;
            }
        }

        ecs_entity_t constraint = scene_add_constraint_to_sketch(scene, sketch, src->type,
                                                                 participants, src->participant_count,
                                                                 src->has_value ? src->value : 0.0f,
                                                                 src->driven);
        if (constraint == 0) {
            sketch_script_apply_set_error(out_error, "Failed creating script constraint.");
            return false;
        }
        sketch_script_apply_add_script_identity(scene, constraint, src->id);
        if (!sketch_script_apply_link_add(links, src->id, constraint)) {
            sketch_script_apply_set_error(out_error, "Failed linking script constraint ID.");
            return false;
        }
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
                                                        sketch_script_error_t *out_error) {
    if (!scene || !scene_is_sketch(scene, sketch) || !model) {
        sketch_script_apply_set_error(out_error, "Invalid scene/sketch/model.");
        return false;
    }

    sketch_script_apply_link_table_t links = {0};
    if (!sketch_script_apply_create_entities(scene, sketch, model, &links, out_error)) return false;
    if (!sketch_script_apply_create_constraints(scene, sketch, model, &links, out_error)) return false;
    scene_refresh_sketch_metadata(scene, sketch);
    return true;
}

static inline bool sketch_script_apply_validate_model_links(const sketch_script_model_t *model,
                                                            sketch_script_error_t *out_error) {
    if (!model) return false;
    for (uint32_t i = 0; i < model->constraint_count; i++) {
        const sketch_script_constraint_model_t *constraint = &model->constraints[i];
        if (constraint->participant_count < constraint_type_min_participants(constraint->type)) {
            sketch_script_apply_set_error(out_error, "Constraint participant count below minimum.");
            return false;
        }
        constraint_selection_signature_t sig = {0};
        sig.count = constraint->participant_count;
        for (uint32_t p = 0; p < constraint->participant_count; p++) {
            bool found = false;
            for (uint32_t e = 0; e < model->entity_count; e++) {
                if (strcmp(model->entities[e].id, constraint->participants[p]) != 0) continue;
                found = true;
                if (model->entities[e].kind == SKETCH_SCRIPT_ENTITY_POINT) {
                    sig.geometry_types[p] = GEOM_POINT;
                } else if (model->entities[e].kind == SKETCH_SCRIPT_ENTITY_LINE) {
                    sig.geometry_types[p] = GEOM_LINE;
                } else {
                    sig.geometry_types[p] = GEOM_ARC;
                }
                sig.roles[p] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
                break;
            }
            if (!found) {
                sketch_script_apply_set_error(out_error, "Unresolved participant reference.");
                return false;
            }
        }
        if (!constraint_type_is_selection_legal(&sig, constraint->type)) {
            sketch_script_apply_set_error(out_error, "Constraint participants are not legal for type.");
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
    return scene_is_sketch(scene, sketch);
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

    ecs_entity_t previous_children[1024] = {0};
    int previous_count = sketch_script_snapshot_sketch_children(scene, sketch, previous_children, 1024);
    sketch_script_remove_children(scene, previous_children, previous_count);

    ecs_entity_t created_children[1024] = {0};
    int created_before = sketch_script_snapshot_sketch_children(scene, sketch, created_children, 1024);
    (void)created_before;

    if (!sketch_script_apply_model_on_sketch(scene, sketch, &model, out_error)) {
        ecs_entity_t created_after[1024] = {0};
        int created_count = sketch_script_snapshot_sketch_children(scene, sketch, created_after, 1024);
        sketch_script_remove_children(scene, created_after, created_count);
        return false;
    }
    return true;
}

#endif // SKETCH_SCRIPT_APPLY_H
