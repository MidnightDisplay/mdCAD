//------------------------------------------------------------------------------
// sketch_script_emit.h - Deterministic scene -> script emitter
//------------------------------------------------------------------------------
#ifndef SKETCH_SCRIPT_EMIT_H
#define SKETCH_SCRIPT_EMIT_H

#include "../ecs/ecs_scene.h"
#include "../components/script_identity_comp.h"
#include "sketch_script_capability_registry.h"
#include "sketch_script_parse.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    ecs_entity_t entity;
    geometry_type_t type;
    char id[SCRIPT_LOCAL_ID_MAX];
} sketch_script_emit_geom_entry_t;

typedef struct {
    ecs_entity_t entity;
    constraint_type_t type;
    char id[SCRIPT_LOCAL_ID_MAX];
} sketch_script_emit_constraint_entry_t;

static inline int sketch_script_emit_compare_id(const char *a, const char *b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    return strcmp(a, b);
}

static inline int sketch_script_emit_compare_descriptor(const constraint_participant_descriptor_t *a,
                                                        const constraint_participant_descriptor_t *b) {
    if (!a || !b) return 0;
    if (a->entity < b->entity) return -1;
    if (a->entity > b->entity) return 1;
    if (a->role < b->role) return -1;
    if (a->role > b->role) return 1;
    if (a->sub_index < b->sub_index) return -1;
    if (a->sub_index > b->sub_index) return 1;
    return 0;
}

static inline void sketch_script_emit_format_number(double value,
                                                    char *out,
                                                    size_t out_size) {
    if (!out || out_size == 0) return;
    snprintf(out, out_size, "%.6f", value);
    out[out_size - 1] = '\0';
    char *dot = strchr(out, '.');
    if (!dot) return;
    char *end = out + strlen(out) - 1;
    while (end > dot && *end == '0') {
        *end-- = '\0';
    }
    if (end == dot) {
        *end = '\0';
    }
}

static inline void sketch_script_emit_append(char *dst,
                                             size_t dst_size,
                                             size_t *offset,
                                             const char *text) {
    if (!dst || !offset || !text || *offset >= dst_size) return;
    size_t remaining = dst_size - *offset;
    int wrote = snprintf(dst + *offset, remaining, "%s", text);
    if (wrote <= 0) return;
    size_t w = (size_t)wrote;
    if (w >= remaining) {
        *offset = dst_size - 1;
        dst[dst_size - 1] = '\0';
        return;
    }
    *offset += w;
}

static inline void sketch_script_emit_appendf(char *dst,
                                              size_t dst_size,
                                              size_t *offset,
                                              const char *fmt,
                                              ...) {
    if (!dst || !offset || !fmt || *offset >= dst_size) return;
    size_t remaining = dst_size - *offset;
    va_list args;
    va_start(args, fmt);
    int wrote = vsnprintf(dst + *offset, remaining, fmt, args);
    va_end(args);
    if (wrote <= 0) return;
    size_t w = (size_t)wrote;
    if (w >= remaining) {
        *offset = dst_size - 1;
        dst[dst_size - 1] = '\0';
        return;
    }
    *offset += w;
}

static inline bool sketch_script_emit_collect(ecs_scene_t *scene,
                                              ecs_entity_t sketch,
                                              sketch_script_emit_geom_entry_t *geoms,
                                              int *geom_count,
                                              sketch_script_emit_constraint_entry_t *constraints,
                                              int *constraint_count,
                                              bool *geom_overflow,
                                              bool *constraint_overflow) {
    if (!scene || !scene_is_sketch(scene, sketch) || !geoms || !geom_count || !constraints || !constraint_count) {
        return false;
    }
    *geom_count = 0;
    *constraint_count = 0;
    if (geom_overflow) *geom_overflow = false;
    if (constraint_overflow) *constraint_overflow = false;

    ecs_iter_t it = ecs_children(scene->world->world, sketch);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            ecs_entity_t child = it.entities[i];
            ScriptIdentityComp *sid = ecs_world_get_script_identity(scene->world, child);
            if (!sid || sid->script_local_id[0] == '\0') continue;
            GeometryComp *g = ecs_world_get_geometry(scene->world, child);
            if (g) {
                if (*geom_count < SKETCH_SCRIPT_MODEL_MAX_ENTITIES) {
                    geoms[*geom_count].entity = child;
                    geoms[*geom_count].type = g->type;
                    strncpy(geoms[*geom_count].id, sid->script_local_id, SCRIPT_LOCAL_ID_MAX - 1);
                    geoms[*geom_count].id[SCRIPT_LOCAL_ID_MAX - 1] = '\0';
                    (*geom_count)++;
                } else if (geom_overflow) {
                    *geom_overflow = true;
                }
            }
            ConstraintComp *c = ecs_world_get_constraint(scene->world, child);
            if (c) {
                if (*constraint_count < SKETCH_SCRIPT_MODEL_MAX_CONSTRAINTS) {
                    constraints[*constraint_count].entity = child;
                    constraints[*constraint_count].type = c->type;
                    strncpy(constraints[*constraint_count].id, sid->script_local_id, SCRIPT_LOCAL_ID_MAX - 1);
                    constraints[*constraint_count].id[SCRIPT_LOCAL_ID_MAX - 1] = '\0';
                    (*constraint_count)++;
                } else if (constraint_overflow) {
                    *constraint_overflow = true;
                }
            }
        }
    }
    return true;
}

static inline int sketch_script_emit_geom_rank(geometry_type_t type) {
    if (type == GEOM_POINT) return 0;
    if (type == GEOM_LINE) return 1;
    if (type == GEOM_ARC) return 2;
    return 3;
}

static inline void sketch_script_emit_sort_geom(sketch_script_emit_geom_entry_t *entries, int count) {
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            int ri = sketch_script_emit_geom_rank(entries[i].type);
            int rj = sketch_script_emit_geom_rank(entries[j].type);
            int cmp = (ri == rj) ? sketch_script_emit_compare_id(entries[i].id, entries[j].id) : (ri - rj);
            if (cmp > 0) {
                sketch_script_emit_geom_entry_t t = entries[i];
                entries[i] = entries[j];
                entries[j] = t;
            }
        }
    }
}

static inline void sketch_script_emit_sort_constraint(sketch_script_emit_constraint_entry_t *entries, int count) {
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            int cmp = (int)entries[i].type - (int)entries[j].type;
            if (cmp == 0) cmp = sketch_script_emit_compare_id(entries[i].id, entries[j].id);
            if (cmp > 0) {
                sketch_script_emit_constraint_entry_t t = entries[i];
                entries[i] = entries[j];
                entries[j] = t;
            }
        }
    }
}

static inline bool sketch_script_emit_for_sketch(ecs_scene_t *scene,
                                                 ecs_entity_t sketch,
                                                 char *out_script,
                                                 size_t out_script_size,
                                                 sketch_script_error_t *out_error) {
    if (!scene || !scene_is_sketch(scene, sketch) || !out_script || out_script_size == 0) {
        if (out_error) {
            out_error->line = 0;
            out_error->column = 0;
            snprintf(out_error->message, sizeof(out_error->message), "Invalid emit arguments.");
            out_error->message[sizeof(out_error->message) - 1] = '\0';
        }
        return false;
    }

    sketch_script_emit_geom_entry_t geoms[SKETCH_SCRIPT_MODEL_MAX_ENTITIES] = {0};
    sketch_script_emit_constraint_entry_t constraints[SKETCH_SCRIPT_MODEL_MAX_CONSTRAINTS] = {0};
    int geom_count = 0;
    int constraint_count = 0;
    bool geom_overflow = false;
    bool constraint_overflow = false;
    if (!sketch_script_emit_collect(scene, sketch, geoms, &geom_count, constraints, &constraint_count,
                                    &geom_overflow, &constraint_overflow)) {
        if (out_error) {
            out_error->line = 0;
            out_error->column = 0;
            snprintf(out_error->message, sizeof(out_error->message), "Failed to collect sketch for emit.");
            out_error->message[sizeof(out_error->message) - 1] = '\0';
        }
        return false;
    }
    if (geom_overflow) {
        if (out_error) {
            out_error->line = 0;
            out_error->column = 0;
            snprintf(out_error->message, sizeof(out_error->message), "too many entities in script model.");
            out_error->message[sizeof(out_error->message) - 1] = '\0';
        }
        return false;
    }
    if (constraint_overflow) {
        if (out_error) {
            out_error->line = 0;
            out_error->column = 0;
            snprintf(out_error->message, sizeof(out_error->message), "too many constraints in script model.");
            out_error->message[sizeof(out_error->message) - 1] = '\0';
        }
        return false;
    }
    sketch_script_emit_sort_geom(geoms, geom_count);
    sketch_script_emit_sort_constraint(constraints, constraint_count);

    out_script[0] = '\0';
    size_t off = 0;
    sketch_script_emit_append(out_script, out_script_size, &off, "return {\n");
    int io_count = scene_script_io_descriptor_count(scene, sketch);
    if (io_count > 0) {
        sketch_script_emit_append(out_script, out_script_size, &off, "  inputs = {\n");
        for (int i = 0; i < io_count; i++) {
            scene_script_io_descriptor_t desc = {0};
            if (!scene_script_io_descriptor_at(scene, sketch, i, &desc) || !desc.is_input) continue;
            char value_text[32] = {0};
            sketch_script_emit_format_number(desc.value, value_text, sizeof(value_text));
            sketch_script_emit_appendf(out_script, out_script_size, &off,
                "    { id = \"%s\", value = %s", desc.id, value_text);
            if (desc.has_min) {
                char min_text[32] = {0};
                sketch_script_emit_format_number(desc.min_value, min_text, sizeof(min_text));
                sketch_script_emit_appendf(out_script, out_script_size, &off, ", min = %s", min_text);
            }
            if (desc.has_max) {
                char max_text[32] = {0};
                sketch_script_emit_format_number(desc.max_value, max_text, sizeof(max_text));
                sketch_script_emit_appendf(out_script, out_script_size, &off, ", max = %s", max_text);
            }
            if (desc.has_step) {
                char step_text[32] = {0};
                sketch_script_emit_format_number(desc.step_value, step_text, sizeof(step_text));
                sketch_script_emit_appendf(out_script, out_script_size, &off, ", step = %s", step_text);
            }
            bool has_more = false;
            for (int j = i + 1; j < io_count; j++) {
                scene_script_io_descriptor_t next = {0};
                if (scene_script_io_descriptor_at(scene, sketch, j, &next) && next.is_input) {
                    has_more = true;
                    break;
                }
            }
            sketch_script_emit_appendf(out_script, out_script_size, &off, " }%s\n", has_more ? "," : "");
        }
        sketch_script_emit_append(out_script, out_script_size, &off, "  },\n");
        sketch_script_emit_append(out_script, out_script_size, &off, "  outputs = {\n");
        for (int i = 0; i < io_count; i++) {
            scene_script_io_descriptor_t desc = {0};
            if (!scene_script_io_descriptor_at(scene, sketch, i, &desc) || desc.is_input) continue;
            char value_text[32] = {0};
            sketch_script_emit_format_number(desc.value, value_text, sizeof(value_text));
            bool has_more = false;
            for (int j = i + 1; j < io_count; j++) {
                scene_script_io_descriptor_t next = {0};
                if (scene_script_io_descriptor_at(scene, sketch, j, &next) && !next.is_input) {
                    has_more = true;
                    break;
                }
            }
            sketch_script_emit_appendf(out_script, out_script_size, &off,
                "    { id = \"%s\", value = %s }%s\n",
                desc.id, value_text, has_more ? "," : "");
        }
        sketch_script_emit_append(out_script, out_script_size, &off, "  },\n");
    }
    sketch_script_emit_append(out_script, out_script_size, &off, "  entities = {\n");
    for (int i = 0; i < geom_count; i++) {
        GeometryComp *g = ecs_world_get_geometry(scene->world, geoms[i].entity);
        if (!g) continue;
        char a[32], b[32], c[32], d[32];
        if (g->type == GEOM_POINT) {
            sketch_script_emit_format_number(g->data.point.point.x, a, sizeof(a));
            sketch_script_emit_format_number(g->data.point.point.y, b, sizeof(b));
            sketch_script_emit_format_number(g->data.point.point.z, c, sizeof(c));
            char cr[32], cg[32], cb[32], ca[32];
            sketch_script_emit_format_number(g->color.x, cr, sizeof(cr));
            sketch_script_emit_format_number(g->color.y, cg, sizeof(cg));
            sketch_script_emit_format_number(g->color.z, cb, sizeof(cb));
            sketch_script_emit_format_number(g->color.w, ca, sizeof(ca));
            sketch_script_emit_appendf(out_script, out_script_size, &off,
                "    { id = \"%s\", type = \"point\", point = {%s, %s, %s}, color = {%s, %s, %s, %s} }%s\n",
                geoms[i].id, a, b, c, cr, cg, cb, ca, (i + 1 < geom_count) ? "," : "");
        } else if (g->type == GEOM_LINE) {
            sketch_script_emit_format_number(g->data.line.a.x, a, sizeof(a));
            sketch_script_emit_format_number(g->data.line.a.y, b, sizeof(b));
            sketch_script_emit_format_number(g->data.line.a.z, c, sizeof(c));
            sketch_script_emit_format_number(g->data.line.b.x, d, sizeof(d));
            char e[32], f[32];
            sketch_script_emit_format_number(g->data.line.b.y, e, sizeof(e));
            sketch_script_emit_format_number(g->data.line.b.z, f, sizeof(f));
            char cr[32], cg[32], cb[32], ca[32];
            sketch_script_emit_format_number(g->color.x, cr, sizeof(cr));
            sketch_script_emit_format_number(g->color.y, cg, sizeof(cg));
            sketch_script_emit_format_number(g->color.z, cb, sizeof(cb));
            sketch_script_emit_format_number(g->color.w, ca, sizeof(ca));
            sketch_script_emit_appendf(out_script, out_script_size, &off,
                "    { id = \"%s\", type = \"line\", a = {%s, %s, %s}, b = {%s, %s, %s}, color = {%s, %s, %s, %s} }%s\n",
                geoms[i].id, a, b, c, d, e, f, cr, cg, cb, ca, (i + 1 < geom_count) ? "," : "");
        } else if (g->type == GEOM_ARC) {
            const bool is_circle = fabsf(g->data.arc.end_angle - g->data.arc.start_angle) >= 6.2830f;
            sketch_script_emit_format_number(g->data.arc.center.x, a, sizeof(a));
            sketch_script_emit_format_number(g->data.arc.center.y, b, sizeof(b));
            sketch_script_emit_format_number(g->data.arc.center.z, c, sizeof(c));
            sketch_script_emit_format_number(g->data.arc.radius, d, sizeof(d));
            char cr[32], cg[32], cb[32], ca[32];
            sketch_script_emit_format_number(g->color.x, cr, sizeof(cr));
            sketch_script_emit_format_number(g->color.y, cg, sizeof(cg));
            sketch_script_emit_format_number(g->color.z, cb, sizeof(cb));
            sketch_script_emit_format_number(g->color.w, ca, sizeof(ca));
            if (is_circle) {
                sketch_script_emit_appendf(out_script, out_script_size, &off,
                    "    { id = \"%s\", type = \"circle\", center = {%s, %s, %s}, radius = %s, normal = {0, 0, 1}, color = {%s, %s, %s, %s} }%s\n",
                    geoms[i].id, a, b, c, d, cr, cg, cb, ca, (i + 1 < geom_count) ? "," : "");
            } else {
                char sa[32], ea[32];
                sketch_script_emit_format_number(g->data.arc.start_angle, sa, sizeof(sa));
                sketch_script_emit_format_number(g->data.arc.end_angle, ea, sizeof(ea));
                sketch_script_emit_appendf(out_script, out_script_size, &off,
                    "    { id = \"%s\", type = \"arc\", center = {%s, %s, %s}, radius = %s, start_angle = %s, end_angle = %s, normal = {0, 0, 1}, color = {%s, %s, %s, %s} }%s\n",
                    geoms[i].id, a, b, c, d, sa, ea, cr, cg, cb, ca, (i + 1 < geom_count) ? "," : "");
            }
        }
    }
    sketch_script_emit_append(out_script, out_script_size, &off, "  },\n  constraints = {\n");
    for (int i = 0; i < constraint_count; i++) {
        ConstraintComp *c = ecs_world_get_constraint(scene->world, constraints[i].entity);
        if (!c) continue;
        sketch_script_emit_appendf(out_script, out_script_size, &off,
            "    { id = \"%s\", type = \"%s\", participants = {",
            constraints[i].id, constraint_type_display_name(c->type));
        constraint_participant_descriptor_t sorted[CONSTRAINT_MAX_PARTICIPANTS] = {0};
        uint32_t emit_count = c->participant_count;
        for (uint32_t p = 0; p < emit_count; p++) {
            sorted[p] = c->participant_descriptors[p];
            if (sorted[p].entity == 0) {
                sorted[p] = constraint_participant_descriptor_make(
                    c->participants[p], CONSTRAINT_PARTICIPANT_ROLE_ENTITY, 0);
            }
        }
        for (uint32_t p = 1; p < emit_count; p++) {
            uint32_t j = p;
            while (j > 0 && sketch_script_emit_compare_descriptor(&sorted[j - 1], &sorted[j]) > 0) {
                constraint_participant_descriptor_t tmp = sorted[j - 1];
                sorted[j - 1] = sorted[j];
                sorted[j] = tmp;
                j--;
            }
        }
        for (uint32_t p = 0; p < emit_count; p++) {
            ScriptIdentityComp *sid = ecs_world_get_script_identity(scene->world, (ecs_entity_t)sorted[p].entity);
            sketch_script_emit_appendf(out_script, out_script_size, &off,
                "{ id = \"%s\", role = \"%s\", sub_index = %u }%s",
                (sid && sid->script_local_id[0]) ? sid->script_local_id : "",
                sketch_script_capability_role_name((constraint_participant_role_t)sorted[p].role),
                (unsigned int)sorted[p].sub_index,
                (p + 1 < emit_count) ? ", " : "");
        }
        if (constraint_type_is_dimensional(c->type)) {
            char value_text[32];
            sketch_script_emit_format_number(c->value, value_text, sizeof(value_text));
            sketch_script_emit_appendf(out_script, out_script_size, &off, "}");
            if (c->pair_owner_constraint_entity != 0 && c->paired_constraint_entity != 0) {
                ScriptIdentityComp *owner_sid = ecs_world_get_script_identity(
                    scene->world, (ecs_entity_t)c->pair_owner_constraint_entity);
                ScriptIdentityComp *paired_sid = ecs_world_get_script_identity(
                    scene->world, (ecs_entity_t)c->paired_constraint_entity);
                sketch_script_emit_appendf(out_script, out_script_size, &off,
                    ", pair = { is_owner = %s, owner = \"%s\", paired = \"%s\" }",
                    c->pair_is_owner ? "true" : "false",
                    (owner_sid && owner_sid->script_local_id[0]) ? owner_sid->script_local_id : "",
                    (paired_sid && paired_sid->script_local_id[0]) ? paired_sid->script_local_id : "");
            }
            sketch_script_emit_appendf(out_script, out_script_size, &off,
                ", value = %s, driven = %s }%s\n",
                value_text, c->driven ? "true" : "false", (i + 1 < constraint_count) ? "," : "");
        } else {
            sketch_script_emit_appendf(out_script, out_script_size, &off, "}");
            if (c->pair_owner_constraint_entity != 0 && c->paired_constraint_entity != 0) {
                ScriptIdentityComp *owner_sid = ecs_world_get_script_identity(
                    scene->world, (ecs_entity_t)c->pair_owner_constraint_entity);
                ScriptIdentityComp *paired_sid = ecs_world_get_script_identity(
                    scene->world, (ecs_entity_t)c->paired_constraint_entity);
                sketch_script_emit_appendf(out_script, out_script_size, &off,
                    ", pair = { is_owner = %s, owner = \"%s\", paired = \"%s\" }",
                    c->pair_is_owner ? "true" : "false",
                    (owner_sid && owner_sid->script_local_id[0]) ? owner_sid->script_local_id : "",
                    (paired_sid && paired_sid->script_local_id[0]) ? paired_sid->script_local_id : "");
            }
            sketch_script_emit_appendf(out_script, out_script_size, &off,
                " }%s\n", (i + 1 < constraint_count) ? "," : "");
        }
    }
    sketch_script_emit_append(out_script, out_script_size, &off, "  }\n}\n");
    if (out_error) {
        out_error->line = 0;
        out_error->column = 0;
        out_error->message[0] = '\0';
    }
    return true;
}

static inline bool scene_script_emit_for_sketch(ecs_scene_t *scene,
                                                 ecs_entity_t sketch,
                                                 char *out_script,
                                                 size_t out_script_size,
                                                 sketch_script_error_t *out_error) {
    return sketch_script_emit_for_sketch(scene, sketch, out_script, out_script_size, out_error);
}

static inline bool scene_script_reemit_for_sketch(ecs_scene_t *scene, ecs_entity_t sketch) {
    char *sink = (char*)calloc((size_t)ECS_SCENE_SCRIPT_TEXT_BUFFER_SIZE, sizeof(char));
    if (!sink) {
        return false;
    }
    sketch_script_error_t err = {0};
    bool ok = scene_script_emit_for_sketch(scene, sketch, sink, ECS_SCENE_SCRIPT_TEXT_BUFFER_SIZE, &err);
    free(sink);
    if (ok && scene) {
        scene->script_emit_revision++;
    }
    return ok;
}

static inline uint64_t scene_script_emit_revision(const ecs_scene_t *scene) {
    return scene ? scene->script_emit_revision : 0;
}

#endif // SKETCH_SCRIPT_EMIT_H
