#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include "../scene_serializer.h"
#include "../scripting/sketch_script_runtime.h"
#include "../scripting/sketch_script_contract.h"
#include "../scripting/sketch_script_parse.h"
#include "../scripting/sketch_script_apply.h"
#include "../scripting/sketch_script_emit.h"
#include "../undo_redo_exec.h"
#include "../ui/ui_entity_inspector.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif
void sokol_main(void) {}
#ifdef __cplusplus
}
#endif

static int test_runtime_rejects_non_54(void) {
    sketch_script_lua_version_t v = sketch_script_lua_version_make(5, 3, 6);
    char err[128] = {0};
    if (sketch_script_runtime_assert_lua54(v, err, sizeof(err))) return 1;
    return strstr(err, "Expected Lua 5.4.x baseline") != NULL ? 0 : 1;
}

static int test_contract_decl_validation(void) {
    char err[128] = {0};
    sketch_script_entity_decl_t ok_entity = {
        .type = "line",
        .has_id = true,
        .id = "geometry_1"
    };
    sketch_script_entity_decl_t bad_entity = {
        .type = "mesh",
        .has_id = true,
        .id = "geometry_2"
    };
    if (!sketch_script_contract_validate_entity_decl(&ok_entity, err, sizeof(err))) return 1;
    if (sketch_script_contract_validate_entity_decl(&bad_entity, err, sizeof(err))) return 1;

    sketch_script_constraint_decl_t ok_constraint = {
        .type = "Coincident",
        .has_id = true,
        .id = "constraint_1",
        .participant_count = 2
    };
    if (!sketch_script_contract_validate_constraint_decl(&ok_constraint, err, sizeof(err))) return 1;
    return 0;
}

static int test_script_identity_roundtrip(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    scene.world = &world;
    scene.visible = true;

    ecs_entity_t sketch = ecs_world_create_entity(scene.world);
    SketchComp sketch_comp = sketch_comp_default();
    ecs_world_set_sketch(scene.world, sketch, &sketch_comp);
    vec4_t color = vec4_make(1.0f, 1.0f, 1.0f, 1.0f);
    ecs_entity_t g1 = ecs_world_create_entity(scene.world);
    ecs_entity_t g2 = ecs_world_create_entity(scene.world);
    GeometryComp g1_comp = geometry_comp_point(vec3_make(0, 0, 0), color, 0.01f);
    GeometryComp g2_comp = geometry_comp_point(vec3_make(1, 0, 0), color, 0.01f);
    ecs_world_set_geometry(scene.world, g1, &g1_comp);
    ecs_world_set_geometry(scene.world, g2, &g2_comp);
    ecs_world_set_parent(scene.world, g1, sketch);
    ecs_world_set_parent(scene.world, g2, sketch);

    ecs_entity_t c1 = ecs_world_create_entity(scene.world);
    uint64_t parts[2] = { (uint64_t)g1, (uint64_t)g2 };
    ConstraintComp c1_comp = constraint_comp_make(CONSTRAINT_COINCIDENT, parts, 2, 0.0f, false);
    ecs_world_set_constraint(scene.world, c1, &c1_comp);
    ecs_world_set_parent(scene.world, c1, sketch);
    if (!g1 || !g2 || !c1) return 1;

    scene_normalize_sketch_script_local_ids(&scene, sketch);
    ScriptIdentityComp *g_id = ecs_world_get_script_identity(scene.world, g1);
    ScriptIdentityComp *c_id = ecs_world_get_script_identity(scene.world, c1);
    if (!g_id || !c_id || g_id->script_local_id[0] == '\0' || c_id->script_local_id[0] == '\0') return 1;

    char saved_geom_id[SCRIPT_LOCAL_ID_MAX];
    strncpy(saved_geom_id, g_id->script_local_id, sizeof(saved_geom_id) - 1);
    saved_geom_id[sizeof(saved_geom_id) - 1] = '\0';

    char *json = scene_save_to_string(&scene);
    if (!json) return 1;
    if (strstr(json, "\"script_identity\"") == NULL || strstr(json, "\"script_local_id\"") == NULL) {
        free(json);
        return 1;
    }

    const char *anchor_json =
        "{\n"
        "  \"format\": \"mdcad-scene\",\n"
        "  \"version\": 2,\n"
        "  \"entities\": [\n"
        "    {\n"
        "      \"id\": 1,\n"
        "      \"parent\": null,\n"
        "      \"components\": {\n"
        "        \"transform\": {\"position\": [0,0,0], \"rotation\": [0,0,0], \"scale\": [1,1,1]},\n"
        "        \"sketch\": {\"status\": 1, \"color\": [1,1,1,1], \"next_geometry_name_index\": [0,0,0,0,0,0,0,0,0,0], \"next_constraint_name_index\": [0,0,0,0,0,0,0,0,0,0,0,0,0]},\n"
        "        \"script_identity\": {\"script_local_id\": \"sketch_1\"}\n"
        "      }\n"
        "    },\n"
        "    {\n"
        "      \"id\": 2,\n"
        "      \"parent\": 1,\n"
        "      \"components\": {\n"
        "        \"transform\": {\"position\": [0,0,0], \"rotation\": [0,0,0], \"scale\": [1,1,1]},\n"
        "        \"constraint\": {\"type\": 1, \"has_value\": false, \"driven\": false, \"value\": 0, \"display_decimals\": 4, \"participants\": []},\n"
        "        \"script_identity\": {\"script_local_id\": \"constraint_42\"}\n"
        "      }\n"
        "    }\n"
        "  ]\n"
        "}\n";

    int loaded = scene_load_from_string(&scene, anchor_json, true);
    free(json);
    if (loaded < 2) return 1;

    bool found_match = false;
    ecs_query_t *cq = ecs_query(scene.world->world, {
        .terms = {{ .id = scene.world->ConstraintComp_id }}
    });
    ecs_iter_t it = ecs_query_iter(scene.world->world, cq);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            ScriptIdentityComp *id = ecs_world_get_script_identity(scene.world, it.entities[i]);
            if (id && strcmp(id->script_local_id, "constraint_42") == 0) {
                found_match = true;
            }
        }
    }
    ecs_query_fini(cq);

    ecs_world_shutdown(&world);
    return (found_match && saved_geom_id[0] != '\0') ? 0 : 1;
}

static int test_script_apply_reconstructs_supported_scope(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1.0f, 1.0f, 1.0f, 1.0f));
    if (sketch == 0) return 1;

    const char *script_text =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} },\n"
        "    { id = \"geometry_2\", type = \"line\", a = {0, 0, 0}, b = {2, 0, 0} },\n"
        "    { id = \"geometry_3\", type = \"arc\", center = {1, 1, 0}, radius = 1.5, start_angle = 0, end_angle = 3.14159, normal = {0, 0, 1} },\n"
        "    { id = \"geometry_4\", type = \"circle\", center = {3, 2, 0}, radius = 2.0, normal = {0, 0, 1} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_1\", type = \"Coincident\", participants = { { id = \"geometry_1\", role = \"entity\", sub_index = 0 }, { id = \"geometry_2\", role = \"entity\", sub_index = 0 } } },\n"
        "    { id = \"constraint_2\", type = \"Length\", participants = { { id = \"geometry_2\", role = \"entity\", sub_index = 0 } }, value = 5.0, driven = false }\n"
        "  }\n"
        "}";

    sketch_script_error_t error = {0};
    if (!scene_script_apply_commit(&scene, sketch, script_text, &error)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    int geometry_count = scene_count_sketch_geometry(&scene, sketch);
    int constraint_count = scene_count_sketch_constraints(&scene, sketch);
    ecs_world_shutdown(&world);
    return (geometry_count == 4 && constraint_count == 2) ? 0 : 1;
}

static int test_script_apply_resolves_forward_references_two_pass(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1.0f, 1.0f, 1.0f, 1.0f));
    if (sketch == 0) return 1;

    const char *script_text =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_2\", type = \"line\", a = {0, 0, 0}, b = {1, 0, 0} },\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_1\", type = \"Coincident\", participants = { { id = \"geometry_1\", role = \"entity\", sub_index = 0 }, { id = \"geometry_2\", role = \"entity\", sub_index = 0 } } }\n"
        "  }\n"
        "}";

    sketch_script_error_t error = {0};
    if (!scene_script_apply_commit(&scene, sketch, script_text, &error)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    bool found_constraint = false;
    ecs_iter_t it = ecs_children(scene.world->world, sketch);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            ConstraintComp *constraint = ecs_world_get_constraint(scene.world, it.entities[i]);
            if (!constraint) continue;
            if (constraint->participant_count == 2 &&
                constraint->participants[0] != 0 &&
                constraint->participants[1] != 0) {
                found_constraint = true;
            }
        }
    }

    ecs_world_shutdown(&world);
    return found_constraint ? 0 : 1;
}

static int test_script_apply_commit_is_atomic_on_unresolved_reference(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1.0f, 1.0f, 1.0f, 1.0f));
    if (sketch == 0) return 1;

    ecs_entity_t existing = scene_add_point_to_sketch(&scene, sketch, vec3_make(10, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    if (existing == 0) {
        ecs_world_shutdown(&world);
        return 1;
    }

    int geometry_before = scene_count_sketch_geometry(&scene, sketch);
    int constraints_before = scene_count_sketch_constraints(&scene, sketch);

    const char *bad_script =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_1\", type = \"Coincident\", participants = { { id = \"geometry_1\", role = \"entity\", sub_index = 0 }, { id = \"missing_geometry\", role = \"entity\", sub_index = 0 } } }\n"
        "  }\n"
        "}";

    sketch_script_error_t error = {0};
    if (scene_script_apply_commit(&scene, sketch, bad_script, &error)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    int geometry_after = scene_count_sketch_geometry(&scene, sketch);
    int constraints_after = scene_count_sketch_constraints(&scene, sketch);
    ecs_world_shutdown(&world);
    return (geometry_before == geometry_after && constraints_before == constraints_after) ? 0 : 1;
}

static int test_script_preview_parse_preserves_committed_scene_on_failure(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1.0f, 1.0f, 1.0f, 1.0f));
    if (sketch == 0) return 1;

    const char *valid_script =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} },\n"
        "    { id = \"geometry_2\", type = \"line\", a = {0, 0, 0}, b = {1, 0, 0} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_1\", type = \"Coincident\", participants = { { id = \"geometry_1\", role = \"entity\", sub_index = 0 }, { id = \"geometry_2\", role = \"entity\", sub_index = 0 } } }\n"
        "  }\n"
        "}";
    sketch_script_error_t error = {0};
    if (!scene_script_apply_commit(&scene, sketch, valid_script, &error)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    int geometry_before = scene_count_sketch_geometry(&scene, sketch);
    int constraints_before = scene_count_sketch_constraints(&scene, sketch);
    const char *invalid_script =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_1\", type = \"Coincident\", participants = { { id = \"geometry_1\", role = \"entity\", sub_index = 0 }, { id = \"missing\", role = \"entity\", sub_index = 0 } } }\n"
        "  }\n"
        "}";
    if (scene_script_preview_parse(&scene, sketch, invalid_script, &error)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    int geometry_after = scene_count_sketch_geometry(&scene, sketch);
    int constraints_after = scene_count_sketch_constraints(&scene, sketch);
    ecs_world_shutdown(&world);
    return (geometry_before == geometry_after && constraints_before == constraints_after) ? 0 : 1;
}

static int test_script_preview_parse_rejects_illegal_constraint_participants(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1.0f, 1.0f, 1.0f, 1.0f));
    if (sketch == 0) return 1;

    const char *invalid_script =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"line\", a = {0, 0, 0}, b = {1, 0, 0} },\n"
        "    { id = \"geometry_2\", type = \"line\", a = {0, 1, 0}, b = {1, 1, 0} },\n"
        "    { id = \"geometry_3\", type = \"arc\", center = {0, 0, 0}, radius = 1, start_angle = 0, end_angle = 3.14, normal = {0, 0, 1} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_1\", type = \"Perpendicular\", participants = { { id = \"geometry_1\", role = \"entity\", sub_index = 0 }, { id = \"geometry_3\", role = \"entity\", sub_index = 0 } } }\n"
        "  }\n"
        "}";
    sketch_script_error_t error = {0};
    bool ok = scene_script_preview_parse(&scene, sketch, invalid_script, &error);
    ecs_world_shutdown(&world);
    if (ok) return 1;
    return error.message[0] != '\0' ? 0 : 1;
}

static int test_script_roundtrip_parallel_perpendicular_group_constraints_lcon04(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch_a = scene_add_sketch(&scene, "SketchA", "", vec4_make(1.0f, 1.0f, 1.0f, 1.0f));
    ecs_entity_t sketch_b = scene_add_sketch(&scene, "SketchB", "", vec4_make(1.0f, 1.0f, 1.0f, 1.0f));
    if (sketch_a == 0 || sketch_b == 0) {
        ecs_world_shutdown(&world);
        return 1;
    }

    const char *script_text =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"line\", a = {0, 0, 0}, b = {3, 0, 0} },\n"
        "    { id = \"geometry_2\", type = \"line\", a = {0, 1, 0}, b = {2.5, 2.0, 0} },\n"
        "    { id = \"geometry_3\", type = \"line\", a = {0, -1, 0}, b = {2.0, -2.0, 0} },\n"
        "    { id = \"geometry_4\", type = \"line\", a = {1.0, 1.0, 0}, b = {2.0, 2.0, 0} },\n"
        "    { id = \"geometry_5\", type = \"line\", a = {1.0, -1.0, 0}, b = {2.0, -3.0, 0} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_1\", type = \"Parallel\", participants = { { id = \"geometry_1\", role = \"entity\", sub_index = 0 }, { id = \"geometry_2\", role = \"entity\", sub_index = 0 }, { id = \"geometry_3\", role = \"entity\", sub_index = 0 } } },\n"
        "    { id = \"constraint_2\", type = \"Perpendicular\", participants = { { id = \"geometry_1\", role = \"entity\", sub_index = 0 }, { id = \"geometry_4\", role = \"entity\", sub_index = 0 }, { id = \"geometry_5\", role = \"entity\", sub_index = 0 } } }\n"
        "  }\n"
        "}";
    sketch_script_error_t err = {0};
    if (!scene_script_apply_commit(&scene, sketch_a, script_text, &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    int parallel_count_a = 0;
    int perpendicular_count_a = 0;
    ecs_iter_t it = ecs_children(scene.world->world, sketch_a);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            ConstraintComp *constraint = ecs_world_get_constraint(scene.world, it.entities[i]);
            if (!constraint) continue;
            if (constraint->type == CONSTRAINT_PARALLEL && constraint->participant_count == 3) parallel_count_a++;
            if (constraint->type == CONSTRAINT_PERPENDICULAR && constraint->participant_count == 3) perpendicular_count_a++;
        }
    }
    if (parallel_count_a != 1 || perpendicular_count_a != 1) {
        ecs_world_shutdown(&world);
        return 1;
    }

    char emitted[8192] = {0};
    if (!scene_script_emit_for_sketch(&scene, sketch_a, emitted, sizeof(emitted), &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    if (strstr(emitted, "type = \"Parallel\"") == NULL || strstr(emitted, "type = \"Perpendicular\"") == NULL) {
        ecs_world_shutdown(&world);
        return 1;
    }

    if (!scene_script_apply_commit(&scene, sketch_b, emitted, &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    int parallel_count_b = 0;
    int perpendicular_count_b = 0;
    it = ecs_children(scene.world->world, sketch_b);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            ConstraintComp *constraint = ecs_world_get_constraint(scene.world, it.entities[i]);
            if (!constraint) continue;
            if (constraint->type == CONSTRAINT_PARALLEL && constraint->participant_count == 3) parallel_count_b++;
            if (constraint->type == CONSTRAINT_PERPENDICULAR && constraint->participant_count == 3) perpendicular_count_b++;
        }
    }

    ecs_world_shutdown(&world);
    return (parallel_count_b == 1 && perpendicular_count_b == 1) ? 0 : 1;
}

static int test_script_parse_rejects_unexpected_tokens_between_blocks(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1.0f, 1.0f, 1.0f, 1.0f));
    if (sketch == 0) return 1;

    const char *invalid_script =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_1\", type = \"Fixed\", participants = { { id = \"geometry_1\", role = \"entity\", sub_index = 0 } } } xyz\n"
        "  }\n"
        "}";
    sketch_script_error_t error = {0};
    bool ok = scene_script_preview_parse(&scene, sketch, invalid_script, &error);
    ecs_world_shutdown(&world);
    if (ok) return 1;
    return error.message[0] != '\0' ? 0 : 1;
}

static int test_script_apply_commit_keeps_last_valid_scene_on_failure(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1.0f, 1.0f, 1.0f, 1.0f));
    if (sketch == 0) return 1;

    const char *valid_script =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} }\n"
        "  },\n"
        "  constraints = {}\n"
        "}";
    sketch_script_error_t error = {0};
    if (!scene_script_apply_commit(&scene, sketch, valid_script, &error)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    char committed_script[4096] = {0};
    if (!scene_script_emit_for_sketch(&scene, sketch, committed_script, sizeof(committed_script), &error)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    const char *invalid_script =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_1\", type = \"Coincident\", participants = { { id = \"geometry_1\", role = \"entity\", sub_index = 0 }, { id = \"missing_geometry\", role = \"entity\", sub_index = 0 } } }\n"
        "  }\n"
        "}";
    if (scene_script_apply_commit(&scene, sketch, invalid_script, &error)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    char after_failed_apply[4096] = {0};
    if (!scene_script_emit_for_sketch(&scene, sketch, after_failed_apply, sizeof(after_failed_apply), &error)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    ecs_world_shutdown(&world);
    return strcmp(committed_script, after_failed_apply) == 0 ? 0 : 1;
}

static bool script_test_has_child_with_script_id(ecs_scene_t *scene, ecs_entity_t sketch, const char *script_id) {
    if (!scene || sketch == 0 || !script_id) return false;
    ecs_iter_t it = ecs_children(scene->world->world, sketch);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            ScriptIdentityComp *sid = ecs_world_get_script_identity(scene->world, it.entities[i]);
            if (sid && strcmp(sid->script_local_id, script_id) == 0) {
                return true;
            }
        }
    }
    return false;
}

static int test_script_apply_undo_redo_single_step(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    undo_redo_t undo_redo = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);
    undo_redo_init(&undo_redo, &scene, 32);
    scene_script_bind_undo_redo(&scene, &undo_redo);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1.0f, 1.0f, 1.0f, 1.0f));
    if (sketch == 0) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    const char *script_a =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} }\n"
        "  },\n"
        "  constraints = {}\n"
        "}";
    const char *script_b =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} },\n"
        "    { id = \"geometry_2\", type = \"line\", a = {0, 0, 0}, b = {1, 0, 0} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_1\", type = \"Coincident\", participants = { { id = \"geometry_1\", role = \"entity\", sub_index = 0 }, { id = \"geometry_2\", role = \"entity\", sub_index = 0 } } }\n"
        "  }\n"
        "}";
    sketch_script_error_t error = {0};
    if (!scene_script_apply_commit(&scene, sketch, script_a, &error)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    int undo_before = undo_redo_get_undo_count(&undo_redo);
    if (!scene_script_apply_commit(&scene, sketch, script_b, &error)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    int undo_after = undo_redo_get_undo_count(&undo_redo);
    int redo_after_apply = undo_redo_get_redo_count(&undo_redo);
    if ((undo_after - undo_before) != 1 || redo_after_apply != 0) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (!undo_redo_undo(&undo_redo)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (scene_count_sketch_geometry(&scene, sketch) != 1) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (!script_test_has_child_with_script_id(&scene, sketch, "geometry_1") ||
        script_test_has_child_with_script_id(&scene, sketch, "geometry_2") ||
        script_test_has_child_with_script_id(&scene, sketch, "constraint_1")) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (!undo_redo_redo(&undo_redo)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (scene_count_sketch_geometry(&scene, sketch) != 2 ||
        scene_count_sketch_constraints(&scene, sketch) != 1) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (!script_test_has_child_with_script_id(&scene, sketch, "geometry_1") ||
        !script_test_has_child_with_script_id(&scene, sketch, "geometry_2") ||
        !script_test_has_child_with_script_id(&scene, sketch, "constraint_1")) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    undo_redo_shutdown(&undo_redo);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_script_apply_failure_preserves_last_valid_state(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    undo_redo_t undo_redo = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);
    undo_redo_init(&undo_redo, &scene, 32);
    scene_script_bind_undo_redo(&scene, &undo_redo);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1.0f, 1.0f, 1.0f, 1.0f));
    if (sketch == 0) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    const char *valid_script =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} },\n"
        "    { id = \"geometry_2\", type = \"line\", a = {0, 0, 0}, b = {1, 0, 0} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_1\", type = \"Coincident\", participants = { { id = \"geometry_1\", role = \"entity\", sub_index = 0 }, { id = \"geometry_2\", role = \"entity\", sub_index = 0 } } }\n"
        "  }\n"
        "}";
    const char *invalid_script =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} },\n"
        "    { id = \"geometry_2\", type = \"line\", a = {0, 0, 0}, b = {1, 0, 0} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_1\", type = \"Coincident\", participants = { { id = \"geometry_1\", role = \"entity\", sub_index = 0 }, { id = \"missing_geometry\", role = \"entity\", sub_index = 0 } } }\n"
        "  }\n"
        "}";

    sketch_script_error_t error = {0};
    if (!scene_script_apply_commit(&scene, sketch, valid_script, &error)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    int geometry_before = scene_count_sketch_geometry(&scene, sketch);
    int constraints_before = scene_count_sketch_constraints(&scene, sketch);
    bool had_g1_before = script_test_has_child_with_script_id(&scene, sketch, "geometry_1");
    bool had_g2_before = script_test_has_child_with_script_id(&scene, sketch, "geometry_2");
    bool had_c1_before = script_test_has_child_with_script_id(&scene, sketch, "constraint_1");
    int undo_before = undo_redo_get_undo_count(&undo_redo);

    if (scene_script_apply_commit(&scene, sketch, invalid_script, &error)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    int geometry_after = scene_count_sketch_geometry(&scene, sketch);
    int constraints_after = scene_count_sketch_constraints(&scene, sketch);
    bool had_g1_after = script_test_has_child_with_script_id(&scene, sketch, "geometry_1");
    bool had_g2_after = script_test_has_child_with_script_id(&scene, sketch, "geometry_2");
    bool had_c1_after = script_test_has_child_with_script_id(&scene, sketch, "constraint_1");
    int undo_after = undo_redo_get_undo_count(&undo_redo);
    int redo_after = undo_redo_get_redo_count(&undo_redo);

    undo_redo_shutdown(&undo_redo);
    ecs_world_shutdown(&world);
    return (geometry_before == geometry_after &&
            constraints_before == constraints_after &&
            had_g1_before == had_g1_after &&
            had_g2_before == had_g2_after &&
            had_c1_before == had_c1_after &&
            undo_before == undo_after &&
            redo_after == 0) ? 0 : 1;
}

static int test_script_apply_respects_undo_suppression_flag(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    undo_redo_t undo_redo = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);
    undo_redo_init(&undo_redo, &scene, 32);
    scene_script_bind_undo_redo(&scene, &undo_redo);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    const char *script_a =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} }\n"
        "  },\n"
        "  constraints = {}\n"
        "}";
    const char *script_b =
        "return {\n"
        "  inputs = {\n"
        "    { id = \"input_length\", value = 10.0, min = 1.0, max = 20.0, step = 0.5 }\n"
        "  },\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} }\n"
        "  },\n"
        "  constraints = {}\n"
        "}";

    sketch_script_error_t err = {0};
    if (!scene_script_apply_commit(&scene, sketch, script_a, &err)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    int undo_before_suppressed = undo_redo_get_undo_count(&undo_redo);

    scene.script_apply_undo_suppressed = true;
    bool suppressed_ok = scene_script_apply_commit(&scene, sketch, script_b, &err);
    scene.script_apply_undo_suppressed = false;
    if (!suppressed_ok) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    int undo_after_suppressed = undo_redo_get_undo_count(&undo_redo);
    if (undo_after_suppressed != undo_before_suppressed) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (!scene_script_apply_commit(&scene, sketch, script_a, &err)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    int undo_after_normal = undo_redo_get_undo_count(&undo_redo);

    undo_redo_shutdown(&undo_redo);
    ecs_world_shutdown(&world);
    return (undo_after_normal == (undo_after_suppressed + 1)) ? 0 : 1;
}

static int test_script_io_numeric_schema_roundtrip(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) return 1;

    const char *script_text =
        "return {\n"
        "  inputs = {\n"
        "    { id = \"input_length\", value = 10.0, min = 1.0, max = 20.0, step = 0.5 }\n"
        "  },\n"
        "  outputs = {\n"
        "    { id = \"output_span\", value = 7.5 }\n"
        "  },\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} }\n"
        "  },\n"
        "  constraints = {}\n"
        "}";
    sketch_script_error_t err = {0};
    if (!scene_script_apply_commit(&scene, sketch, script_text, &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    char emitted[4096] = {0};
    if (!scene_script_emit_for_sketch(&scene, sketch, emitted, sizeof(emitted), &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    ecs_world_shutdown(&world);
    return (strstr(emitted, "inputs") != NULL &&
            strstr(emitted, "outputs") != NULL &&
            strstr(emitted, "min = 1") != NULL &&
            strstr(emitted, "max = 20") != NULL &&
            strstr(emitted, "step = 0.5") != NULL) ? 0 : 1;
}

static int test_script_io_rejects_non_numeric(void) {
    sketch_script_model_t model = {0};
    sketch_script_error_t err = {0};
    const char *bad_script =
        "return {\n"
        "  inputs = {\n"
        "    { id = \"mode\", value = true }\n"
        "  },\n"
        "  outputs = {},\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} }\n"
        "  },\n"
        "  constraints = {}\n"
        "}";
    if (sketch_script_parse_model(bad_script, &model, &err)) {
        return 1;
    }
    return strstr(err.message, "numeric") != NULL ? 0 : 1;
}

static int test_script_io_live_edit_uses_transaction_pipeline(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    undo_redo_t undo_redo = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);
    undo_redo_init(&undo_redo, &scene, 32);
    scene_script_bind_undo_redo(&scene, &undo_redo);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    const char *script_text =
        "return {\n"
        "  inputs = {\n"
        "    { id = \"input_length\", value = 10.0, min = 1.0, max = 20.0, step = 0.5 }\n"
        "  },\n"
        "  outputs = {\n"
        "    { id = \"output_span\", value = 7.5 }\n"
        "  },\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} }\n"
        "  },\n"
        "  constraints = {}\n"
        "}";
    sketch_script_error_t err = {0};
    if (!scene_script_apply_commit(&scene, sketch, script_text, &err)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    double value_before = 0.0;
    bool is_input_before = false;
    if (!scene_script_io_read_value(&scene, sketch, "input_length", &value_before, &is_input_before)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (!is_input_before || value_before != 10.0) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    int undo_before = undo_redo_get_undo_count(&undo_redo);
    if (!scene_script_io_apply_input_value(&scene, sketch, "input_length", 12.5, &err)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    int undo_after = undo_redo_get_undo_count(&undo_redo);

    double value_after = 0.0;
    bool is_input_after = false;
    if (!scene_script_io_read_value(&scene, sketch, "input_length", &value_after, &is_input_after)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (!is_input_after || value_after != 12.5) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (scene_script_io_apply_input_value(&scene, sketch, "missing_input", 99.0, &err)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    double value_after_failed_edit = 0.0;
    bool is_input_after_failed_edit = false;
    if (!scene_script_io_read_value(&scene, sketch, "input_length", &value_after_failed_edit, &is_input_after_failed_edit)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    undo_redo_shutdown(&undo_redo);
    ecs_world_shutdown(&world);
    return (undo_after - undo_before) == 1 &&
           is_input_after_failed_edit &&
           value_after_failed_edit == 12.5 ? 0 : 1;
}

static int test_script_io_live_edit_handles_large_script_buffers(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    undo_redo_t undo_redo = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);
    undo_redo_init(&undo_redo, &scene, 32);
    scene_script_bind_undo_redo(&scene, &undo_redo);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    const char *base_script =
        "return {\n"
        "  inputs = {\n"
        "    { id = \"front_span\", value = 1.5, min = 0.5, max = 3.0, step = 0.05 },\n"
        "    { id = \"hole_depth\", value = 1.0, min = 0.25, max = 2.0, step = 0.05 },\n"
        "    { id = \"include_cube_connectors\", value = 1.0, min = 0.0, max = 1.0, step = 1.0 },\n"
        "    { id = \"include_hole_connectors\", value = 1.0, min = 0.0, max = 1.0, step = 1.0 }\n"
        "  },\n"
        "  outputs = {\n"
        "    { id = \"body_depth\", value = 1.0 },\n"
        "    { id = \"hole_diameter\", value = 0.7 }\n"
        "  },\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} }\n"
        "  },\n"
        "  constraints = {}\n"
        "}";

    sketch_script_error_t err = {0};
    if (!scene_script_apply_commit(&scene, sketch, base_script, &err)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    for (int i = 0; i < 100; i++) {
        float x = (float)(i % 10) * 0.1f;
        float y = (float)((i / 10) % 10) * 0.1f;
        float z = (float)(i / 100) * 0.1f;
        char id_buf[32] = {0};
        snprintf(id_buf, sizeof(id_buf), "bulk_line_%d", i + 1);
        ecs_entity_t line = scene_add_line_to_sketch(&scene,
                                                     sketch,
                                                     vec3_make(x, y, z),
                                                     vec3_make(x + 1.0f, y, z),
                                                     vec4_make(1, 1, 1, 1),
                                                     0.01f);
        if (line == 0) {
            undo_redo_shutdown(&undo_redo);
            ecs_world_shutdown(&world);
            return 1;
        }
        ecs_world_set_script_identity(scene.world, line, &(ScriptIdentityComp){0});
        ScriptIdentityComp *sid = ecs_world_get_script_identity(scene.world, line);
        if (!sid) {
            undo_redo_shutdown(&undo_redo);
            ecs_world_shutdown(&world);
            return 1;
        }
        snprintf(sid->script_local_id, SCRIPT_LOCAL_ID_MAX, "%s", id_buf);
        sid->script_local_id[SCRIPT_LOCAL_ID_MAX - 1] = '\0';
    }

    if (!scene_script_reemit_for_sketch(&scene, sketch)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    char emitted[ECS_SCENE_SCRIPT_TEXT_BUFFER_SIZE] = {0};
    if (!scene_script_emit_for_sketch(&scene, sketch, emitted, sizeof(emitted), &err)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (strlen(emitted) <= 4096) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (!scene_script_io_apply_input_value(&scene, sketch, "front_span", 1.8, &err)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    double updated = 0.0;
    bool is_input = false;
    if (!scene_script_io_read_value(&scene, sketch, "front_span", &updated, &is_input)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    undo_redo_shutdown(&undo_redo);
    ecs_world_shutdown(&world);
    double delta = updated - 1.8;
    if (delta < 0.0) delta = -delta;
    return (!is_input || delta > 1e-4) ? 1 : 0;
}

static int test_script_io_numeric_input_coalesces_single_undo_step(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    undo_redo_t undo_redo = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);
    undo_redo_init(&undo_redo, &scene, 32);
    scene_script_bind_undo_redo(&scene, &undo_redo);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    const char *script_text =
        "return {\n"
        "  inputs = {\n"
        "    { id = \"input_length\", value = 10.0, min = 1.0, max = 20.0, step = 0.5 }\n"
        "  },\n"
        "  outputs = {\n"
        "    { id = \"output_span\", value = 7.5 }\n"
        "  },\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} }\n"
        "  },\n"
        "  constraints = {}\n"
        "}";
    sketch_script_error_t err = {0};
    if (!scene_script_apply_commit(&scene, sketch, script_text, &err)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    char before_script[ECS_SCENE_SCRIPT_TEXT_BUFFER_SIZE] = {0};
    if (!scene_script_emit_for_sketch(&scene, sketch, before_script, sizeof(before_script), &err)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    scene.script_apply_undo_suppressed = true;
    if (!scene_script_io_apply_input_value(&scene, sketch, "input_length", 12.75, &err)) {
        scene.script_apply_undo_suppressed = false;
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    scene.script_apply_undo_suppressed = false;

    char after_script[ECS_SCENE_SCRIPT_TEXT_BUFFER_SIZE] = {0};
    if (!scene_script_emit_for_sketch(&scene, sketch, after_script, sizeof(after_script), &err)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (strcmp(before_script, after_script) == 0) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    int undo_before_push = undo_redo_get_undo_count(&undo_redo);
    undo_cmd_script_apply_transaction(&undo_redo, sketch, before_script, after_script);
    int undo_after_push = undo_redo_get_undo_count(&undo_redo);
    if ((undo_after_push - undo_before_push) != 1) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (!undo_redo_undo(&undo_redo)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    double reverted_value = 0.0;
    bool reverted_is_input = false;
    if (!scene_script_io_read_value(&scene, sketch, "input_length", &reverted_value, &reverted_is_input)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (!reverted_is_input || reverted_value != 10.0) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    undo_redo_shutdown(&undo_redo);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_script_apply_transaction_uses_transaction_command_type(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    undo_redo_t undo_redo = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);
    undo_redo_init(&undo_redo, &scene, 32);
    scene_script_bind_undo_redo(&scene, &undo_redo);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    const char *script_a =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} }\n"
        "  },\n"
        "  constraints = {}\n"
        "}";
    const char *script_b =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {2, 0, 0} }\n"
        "  },\n"
        "  constraints = {}\n"
        "}";
    sketch_script_error_t err = {0};
    if (!scene_script_apply_commit(&scene, sketch, script_a, &err)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    int undo_before = undo_redo_get_undo_count(&undo_redo);
    if (!scene_script_apply_commit(&scene, sketch, script_b, &err)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    int undo_after = undo_redo_get_undo_count(&undo_redo);
    if ((undo_after - undo_before) != 1 || undo_redo.current <= 0) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (undo_redo.commands[undo_redo.current - 1].type != CMD_SCRIPT_APPLY_TRANSACTION) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    undo_redo_shutdown(&undo_redo);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_script_apply_transaction_noop_does_not_push_undo(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    undo_redo_t undo_redo = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);
    undo_redo_init(&undo_redo, &scene, 32);
    scene_script_bind_undo_redo(&scene, &undo_redo);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }

    const char *script_text =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} }\n"
        "  },\n"
        "  constraints = {}\n"
        "}";
    int undo_before = undo_redo_get_undo_count(&undo_redo);
    undo_cmd_script_apply_transaction(&undo_redo, sketch, script_text, script_text);
    int undo_after = undo_redo_get_undo_count(&undo_redo);

    undo_redo_shutdown(&undo_redo);
    ecs_world_shutdown(&world);
    return (undo_after == undo_before) ? 0 : 1;
}

static int test_script_io_window_request_is_exposed_from_inspector_state(void) {
    selection_buffer_t selection = {0};
    ecs_world_state_t world = {0};
    ui_entity_inspector_state_t inspector = {0};
    ui_entity_inspector_init(&inspector, &selection, &world);
    ui_entity_inspector_set_sketch_geometry_mutation_callback(&inspector, NULL, NULL);

    ecs_entity_t requested = 0;
    if (ui_entity_inspector_consume_script_io_open_request(&inspector, &requested)) {
        return 1;
    }

    ui_entity_inspector_request_script_io(&inspector, (ecs_entity_t)77);
    if (!ui_entity_inspector_consume_script_io_open_request(&inspector, &requested)) {
        return 1;
    }
    if (requested != (ecs_entity_t)77) {
        return 1;
    }
    if (ui_entity_inspector_consume_script_io_open_request(&inspector, &requested)) {
        return 1;
    }
    return 0;
}

static int test_script_apply_preserves_labels_by_script_identity(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) {
        ecs_world_shutdown(&world);
        return 1;
    }

    const char *script_text =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_101\", type = \"point\", point = {0, 0, 0} },\n"
        "    { id = \"geometry_102\", type = \"line\", a = {0, 0, 0}, b = {1, 0, 0} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_201\", type = \"Coincident\", participants = { { id = \"geometry_101\", role = \"entity\", sub_index = 0 }, { id = \"geometry_102\", role = \"entity\", sub_index = 0 } } }\n"
        "  }\n"
        "}";
    sketch_script_error_t err = {0};
    if (!scene_script_apply_commit(&scene, sketch, script_text, &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_entity_t geometry_a = 0;
    ecs_entity_t constraint_a = 0;
    ecs_iter_t it = ecs_children(scene.world->world, sketch);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            ScriptIdentityComp *sid = ecs_world_get_script_identity(scene.world, it.entities[i]);
            if (!sid) continue;
            if (strcmp(sid->script_local_id, "geometry_101") == 0) {
                geometry_a = it.entities[i];
            } else if (strcmp(sid->script_local_id, "constraint_201") == 0) {
                constraint_a = it.entities[i];
            }
        }
    }
    if (geometry_a == 0 || constraint_a == 0) {
        ecs_world_shutdown(&world);
        return 1;
    }

    LabelComp custom_geom = label_comp_make("VertexAnchor", "manual-geom");
    LabelComp custom_constraint = label_comp_make("LockRelation", "manual-constraint");
    ecs_world_set_label(scene.world, geometry_a, &custom_geom);
    ecs_world_set_label(scene.world, constraint_a, &custom_constraint);

    if (!scene_script_apply_commit(&scene, sketch, script_text, &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    bool geom_ok = false;
    bool constraint_ok = false;
    it = ecs_children(scene.world->world, sketch);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            ScriptIdentityComp *sid = ecs_world_get_script_identity(scene.world, it.entities[i]);
            LabelComp *label = ecs_world_get_label(scene.world, it.entities[i]);
            if (!sid || !label) continue;
            if (strcmp(sid->script_local_id, "geometry_101") == 0) {
                geom_ok = strcmp(label->name, "VertexAnchor") == 0;
            } else if (strcmp(sid->script_local_id, "constraint_201") == 0) {
                constraint_ok = strcmp(label->name, "LockRelation") == 0;
            }
        }
    }

    ecs_world_shutdown(&world);
    return (geom_ok && constraint_ok) ? 0 : 1;
}

static int test_script_editor_launch_request_is_exposed_from_inspector_state(void) {
    selection_buffer_t selection = {0};
    ecs_world_state_t world = {0};
    ui_entity_inspector_state_t inspector = {0};
    ui_entity_inspector_init(&inspector, &selection, &world);
    ui_entity_inspector_set_sketch_geometry_mutation_callback(&inspector, NULL, NULL);

    ecs_entity_t requested = 0;
    if (ui_entity_inspector_consume_script_editor_open_request(&inspector, &requested)) {
        return 1;
    }

    ui_entity_inspector_request_script_editor(&inspector, (ecs_entity_t)42);
    if (!ui_entity_inspector_consume_script_editor_open_request(&inspector, &requested)) {
        return 1;
    }
    if (requested != (ecs_entity_t)42) {
        return 1;
    }
    if (ui_entity_inspector_consume_script_editor_open_request(&inspector, &requested)) {
        return 1;
    }
    return 0;
}

static void test_inspector_geometry_manager_mutation_callback(void *user_data) {
    if (!user_data) return;
    int *counter = (int*)user_data;
    (*counter)++;
}

static int test_geometry_manager_mutations_trigger_dirty_callback(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    selection_buffer_t selection = {0};
    undo_redo_t undo_redo = {0};
    ui_entity_inspector_state_t inspector = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);
    selection_init(&selection, &world);
    undo_redo_init(&undo_redo, &scene, 32);
    ui_entity_inspector_init(&inspector, &selection, &world);
    ui_entity_inspector_set_undo_redo(&inspector, &undo_redo);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) {
        undo_redo_shutdown(&undo_redo);
        selection_shutdown(&selection);
        ecs_world_shutdown(&world);
        return 1;
    }

    int callback_count = 0;
    ui_entity_inspector_set_sketch_geometry_mutation_callback(
        &inspector,
        test_inspector_geometry_manager_mutation_callback,
        &callback_count);

    SketchComp *sketch_comp = ecs_world_get_sketch(&world, sketch);
    if (!sketch_comp) {
        undo_redo_shutdown(&undo_redo);
        selection_shutdown(&selection);
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_entity_t point = ui_entity_inspector_geometry_manager_add_point(&inspector, &scene, sketch, sketch_comp->color);
    ecs_entity_t line = ui_entity_inspector_geometry_manager_add_line(&inspector, &scene, sketch, sketch_comp->color);
    ecs_entity_t arc = ui_entity_inspector_geometry_manager_add_arc(&inspector, &scene, sketch, sketch_comp->color);
    ecs_entity_t circle = ui_entity_inspector_geometry_manager_add_circle(&inspector, &scene, sketch, sketch_comp->color);
    if (point == 0 || line == 0 || arc == 0 || circle == 0 || callback_count != 4) {
        undo_redo_shutdown(&undo_redo);
        selection_shutdown(&selection);
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_entity_t fix_targets[2] = { point, line };
    int fixed_count = ui_entity_inspector_geometry_manager_bulk_set_fixed(&inspector, &world, &scene, sketch, fix_targets, 2, true);
    int unfixed_count = ui_entity_inspector_geometry_manager_bulk_set_fixed(&inspector, &world, &scene, sketch, fix_targets, 2, false);
    if (fixed_count != 2 || unfixed_count != 2 || callback_count != 6) {
        undo_redo_shutdown(&undo_redo);
        selection_shutdown(&selection);
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_entity_t delete_targets[1] = { circle };
    int deleted_count = ui_entity_inspector_geometry_manager_bulk_delete(
        &inspector, &world, &scene, &selection, delete_targets, 1);
    if (deleted_count != 1 || callback_count != 7) {
        undo_redo_shutdown(&undo_redo);
        selection_shutdown(&selection);
        ecs_world_shutdown(&world);
        return 1;
    }

    int no_op_count = ui_entity_inspector_geometry_manager_bulk_set_fixed(&inspector, &world, &scene, sketch, NULL, 0, true);
    undo_redo_shutdown(&undo_redo);
    selection_shutdown(&selection);
    ecs_world_shutdown(&world);
    return (no_op_count == 0 && callback_count == 7) ? 0 : 1;
}

static int test_script_emit_orders_by_type_and_script_id(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) return 1;

    ecs_entity_t line = scene_add_line_to_sketch(&scene, sketch, vec3_make(0, 0, 0), vec3_make(1, 0, 0), vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t point = scene_add_point_to_sketch(&scene, sketch, vec3_make(0, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t arc = scene_add_arc_to_sketch(&scene, sketch, vec3_make(1, 1, 0), 1.0f, 0.0f, 1.0f, vec3_make(0, 0, 1), vec4_make(1, 1, 1, 1), 1.0f);
    if (!line || !point || !arc) {
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_world_set_script_identity(scene.world, line, &(ScriptIdentityComp){.script_local_id = "geometry_2"});
    ecs_world_set_script_identity(scene.world, point, &(ScriptIdentityComp){.script_local_id = "geometry_1"});
    ecs_world_set_script_identity(scene.world, arc, &(ScriptIdentityComp){.script_local_id = "geometry_3"});

    char script[4096] = {0};
    sketch_script_error_t err = {0};
    if (!scene_script_emit_for_sketch(&scene, sketch, script, sizeof(script), &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    char *point_pos = strstr(script, "id = \"geometry_1\"");
    char *line_pos = strstr(script, "id = \"geometry_2\"");
    char *arc_pos = strstr(script, "id = \"geometry_3\"");
    ecs_world_shutdown(&world);
    if (!point_pos || !line_pos || !arc_pos) return 1;
    return (point_pos < line_pos && line_pos < arc_pos) ? 0 : 1;
}

static int test_script_emit_formats_numbers_without_scientific_notation(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) return 1;
    ecs_entity_t point = scene_add_point_to_sketch(&scene, sketch, vec3_make(1.500000f, 0.000123f, 0.0f), vec4_make(1, 1, 1, 1), 0.01f);
    if (point == 0) {
        ecs_world_shutdown(&world);
        return 1;
    }
    ecs_world_set_script_identity(scene.world, point, &(ScriptIdentityComp){.script_local_id = "geometry_1"});

    char script[4096] = {0};
    sketch_script_error_t err = {0};
    if (!scene_script_emit_for_sketch(&scene, sketch, script, sizeof(script), &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    ecs_world_shutdown(&world);
    if (strstr(script, "e+") || strstr(script, "e-") || strstr(script, "E+") || strstr(script, "E-")) return 1;
    if (strstr(script, "1.5") == NULL) return 1;
    return 0;
}

static int test_script_emit_noop_stability(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) return 1;
    ecs_entity_t point = scene_add_point_to_sketch(&scene, sketch, vec3_make(0, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    if (point == 0) {
        ecs_world_shutdown(&world);
        return 1;
    }
    ecs_world_set_script_identity(scene.world, point, &(ScriptIdentityComp){.script_local_id = "geometry_1"});

    char first[4096] = {0};
    char second[4096] = {0};
    sketch_script_error_t err = {0};
    if (!scene_script_emit_for_sketch(&scene, sketch, first, sizeof(first), &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    scene_refresh_sketch_metadata(&scene, sketch);
    if (!scene_script_emit_for_sketch(&scene, sketch, second, sizeof(second), &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    ecs_world_shutdown(&world);
    return strcmp(first, second) == 0 ? 0 : 1;
}

static int test_script_reemit_revision_changes_on_mutation(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) return 1;
    uint64_t rev0 = scene_script_emit_revision(&scene);
    ecs_entity_t point = scene_add_point_to_sketch(&scene, sketch, vec3_make(0, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    if (point == 0) {
        ecs_world_shutdown(&world);
        return 1;
    }
    uint64_t rev1 = scene_script_emit_revision(&scene);
    ecs_world_shutdown(&world);
    return rev1 > rev0 ? 0 : 1;
}

static int test_script_emit_includes_incremental_manual_additions_after_script_activation(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) return 1;

    const char *seed_script =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"line\", a = {0, 0, 0}, b = {1, 0, 0} }\n"
        "  },\n"
        "  constraints = {}\n"
        "}";
    sketch_script_error_t err = {0};
    if (!scene_script_apply_commit(&scene, sketch, seed_script, &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_entity_t line_b = scene_add_line_to_sketch(&scene, sketch,
                                                    vec3_make(0, 1, 0),
                                                    vec3_make(1, 1, 0),
                                                    vec4_make(1, 1, 1, 1), 1.0f);
    if (line_b == 0) {
        ecs_world_shutdown(&world);
        return 1;
    }
    ScriptIdentityComp *line_b_sid = ecs_world_get_script_identity(scene.world, line_b);
    if (!line_b_sid || line_b_sid->script_local_id[0] == '\0') {
        ecs_world_shutdown(&world);
        return 1;
    }

    constraint_participant_descriptor_t participants[2] = {
        constraint_participant_descriptor_make((uint64_t)line_b, CONSTRAINT_PARTICIPANT_ROLE_ENTITY, 0),
        constraint_participant_descriptor_make((uint64_t)line_b, CONSTRAINT_PARTICIPANT_ROLE_ENTITY, 0)
    };
    ecs_entity_t c = scene_add_constraint_to_sketch_with_descriptors(
        &scene, sketch, CONSTRAINT_FIXED, participants, 1, 0.0f, false);
    if (c == 0) {
        ecs_world_shutdown(&world);
        return 1;
    }
    ScriptIdentityComp *c_sid = ecs_world_get_script_identity(scene.world, c);
    if (!c_sid || c_sid->script_local_id[0] == '\0') {
        ecs_world_shutdown(&world);
        return 1;
    }

    char emitted[8192] = {0};
    if (!scene_script_emit_for_sketch(&scene, sketch, emitted, sizeof(emitted), &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    bool has_seed = strstr(emitted, "id = \"geometry_1\"") != NULL;
    bool has_new_geom = strstr(emitted, line_b_sid->script_local_id) != NULL;
    bool has_new_constraint = strstr(emitted, c_sid->script_local_id) != NULL;
    ecs_world_shutdown(&world);
    return (has_seed && has_new_geom && has_new_constraint) ? 0 : 1;
}

static int test_registry_parity_deterministic_contract_error(void) {
    sketch_script_entity_decl_t bad_entity = {
        .type = "mesh",
        .has_id = true,
        .id = "geometry_bad"
    };
    char err[192] = {0};
    if (sketch_script_contract_validate_entity_decl(&bad_entity, err, sizeof(err))) return 1;
    return strstr(err, "capability registry") != NULL ? 0 : 1;
}

static int test_descriptor_roundtrip_preserves_role_and_sub_index(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) return 1;

    const char *script =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"line\", a = {0, 0, 0}, b = {2, 0, 0}, color = {0.2, 0.3, 0.4, 1} },\n"
        "    { id = \"geometry_2\", type = \"arc\", center = {2, 0, 0}, radius = 1, start_angle = 0, end_angle = 2, normal = {0, 0, 1}, color = {0.5, 0.4, 0.3, 1} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_1\", type = \"Line-End Arc-End Tangency\", participants = {"
        "{ id = \"geometry_1\", role = \"point_b\", sub_index = 0 }, "
        "{ id = \"geometry_2\", role = \"point_a\", sub_index = 3 } } }\n"
        "  }\n"
        "}";

    sketch_script_error_t err = {0};
    if (!scene_script_apply_commit(&scene, sketch, script, &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    char emitted[8192] = {0};
    if (!scene_script_emit_for_sketch(&scene, sketch, emitted, sizeof(emitted), &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    bool has_roles = strstr(emitted, "role = \"point_a\"") != NULL &&
                     strstr(emitted, "role = \"point_b\"") != NULL &&
                     strstr(emitted, "sub_index = 3") != NULL;
    if (!has_roles) {
        ecs_world_shutdown(&world);
        return 1;
    }
    if (!scene_script_apply_commit(&scene, sketch, emitted, &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    bool role_preserved = false;
    ecs_iter_t it = ecs_children(scene.world->world, sketch);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            ConstraintComp *constraint = ecs_world_get_constraint(scene.world, it.entities[i]);
            if (!constraint || constraint->type != CONSTRAINT_LINE_ARC_ENDPOINT_TANGENCY) continue;
            if (constraint->participant_count != 2) continue;
            constraint_participant_role_t r0 = (constraint_participant_role_t)constraint->participant_descriptors[0].role;
            constraint_participant_role_t r1 = (constraint_participant_role_t)constraint->participant_descriptors[1].role;
            uint8_t s0 = constraint->participant_descriptors[0].sub_index;
            uint8_t s1 = constraint->participant_descriptors[1].sub_index;
            role_preserved = ((r0 == CONSTRAINT_PARTICIPANT_ROLE_POINT_A && r1 == CONSTRAINT_PARTICIPANT_ROLE_POINT_B &&
                               s0 == 3 && s1 == 0) ||
                              (r0 == CONSTRAINT_PARTICIPANT_ROLE_POINT_B && r1 == CONSTRAINT_PARTICIPANT_ROLE_POINT_A &&
                               s0 == 0 && s1 == 3));
        }
    }
    ecs_world_shutdown(&world);
    return role_preserved ? 0 : 1;
}

static int test_deterministic_contract_error_atomic_reject(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) return 1;
    ecs_entity_t baseline = scene_add_point_to_sketch(&scene, sketch, vec3_make(0, 0, 0), vec4_make(1, 0, 0, 1), 0.01f);
    if (baseline == 0) {
        ecs_world_shutdown(&world);
        return 1;
    }
    int geometry_before = scene_count_sketch_geometry(&scene, sketch);
    int constraints_before = scene_count_sketch_constraints(&scene, sketch);

    const char *bad_script =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"line\", a = {0, 0, 0}, b = {1, 0, 0}, color = {1, 1, 1, 1} },\n"
        "    { id = \"geometry_2\", type = \"arc\", center = {1, 0, 0}, radius = 1, start_angle = 0, end_angle = 1, normal = {0, 0, 1}, color = {1, 1, 1, 1} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_1\", type = \"Line-End Arc-End Tangency\", participants = {\"geometry_1\", \"geometry_2\"} }\n"
        "  }\n"
        "}";

    sketch_script_error_t err = {0};
    bool ok = scene_script_apply_commit(&scene, sketch, bad_script, &err);
    int geometry_after = scene_count_sketch_geometry(&scene, sketch);
    int constraints_after = scene_count_sketch_constraints(&scene, sketch);
    bool taxonomy = strstr(err.message, "descriptor objects") != NULL;
    ecs_world_shutdown(&world);
    return (!ok && taxonomy &&
            geometry_before == geometry_after &&
            constraints_before == constraints_after) ? 0 : 1;
}

static int test_color_roundtrip_and_non_script_color_untouched(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) return 1;
    ecs_entity_t non_script = scene_add_point_to_sketch(&scene, sketch, vec3_make(10, 10, 0), vec4_make(0.9f, 0.1f, 0.2f, 1.0f), 0.01f);
    if (non_script == 0) {
        ecs_world_shutdown(&world);
        return 1;
    }

    const char *script =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0}, color = {0.2, 0.3, 0.4, 0.9} },\n"
        "    { id = \"geometry_2\", type = \"line\", a = {0, 0, 0}, b = {1, 0, 0}, color = {0.1, 0.8, 0.2, 1} },\n"
        "    { id = \"geometry_3\", type = \"arc\", center = {1, 1, 0}, radius = 1, start_angle = 0, end_angle = 1.5, normal = {0, 0, 1}, color = {0.7, 0.2, 0.6, 1} }\n"
        "  },\n"
        "  constraints = {}\n"
        "}";

    sketch_script_error_t err = {0};
    if (!scene_script_apply_commit(&scene, sketch, script, &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    char emitted[8192] = {0};
    if (!scene_script_emit_for_sketch(&scene, sketch, emitted, sizeof(emitted), &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    if (!scene_script_apply_commit(&scene, sketch, emitted, &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    bool colors_ok = false;
    bool non_script_ok = false;
    ecs_iter_t it = ecs_children(scene.world->world, sketch);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            ecs_entity_t child = it.entities[i];
            GeometryComp *g = ecs_world_get_geometry(scene.world, child);
            ScriptIdentityComp *sid = ecs_world_get_script_identity(scene.world, child);
            if (!g) continue;
            if (sid && strcmp(sid->script_local_id, "geometry_1") == 0 && g->type == GEOM_POINT) {
                colors_ok = (fabsf(g->color.x - 0.2f) < 1e-5f &&
                             fabsf(g->color.y - 0.3f) < 1e-5f &&
                             fabsf(g->color.z - 0.4f) < 1e-5f &&
                             fabsf(g->color.w - 0.9f) < 1e-5f);
            }
            if (child == non_script && g->type == GEOM_POINT) {
                non_script_ok = (fabsf(g->color.x - 0.9f) < 1e-5f &&
                                 fabsf(g->color.y - 0.1f) < 1e-5f &&
                                 fabsf(g->color.z - 0.2f) < 1e-5f &&
                                 fabsf(g->color.w - 1.0f) < 1e-5f);
            }
        }
    }
    ecs_world_shutdown(&world);
    return (colors_ok && non_script_ok) ? 0 : 1;
}

static int test_repeat_apply_deterministic(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) return 1;
    const char *seed =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0}, color = {0.1, 0.1, 0.1, 1} },\n"
        "    { id = \"geometry_2\", type = \"line\", a = {0, 0, 0}, b = {1, 0, 0}, color = {0.2, 0.2, 0.2, 1} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_1\", type = \"Length\", participants = { { id = \"geometry_2\", role = \"entity\", sub_index = 0 } }, value = 1, driven = false }\n"
        "  }\n"
        "}";
    sketch_script_error_t err = {0};
    if (!scene_script_apply_commit(&scene, sketch, seed, &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    char canonical[8192] = {0};
    if (!scene_script_emit_for_sketch(&scene, sketch, canonical, sizeof(canonical), &err)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    for (int i = 0; i < 5; i++) {
        if (!scene_script_apply_commit(&scene, sketch, canonical, &err)) {
            ecs_world_shutdown(&world);
            return 1;
        }
        char emitted[8192] = {0};
        if (!scene_script_emit_for_sketch(&scene, sketch, emitted, sizeof(emitted), &err)) {
            ecs_world_shutdown(&world);
            return 1;
        }
        if (strcmp(canonical, emitted) != 0) {
            ecs_world_shutdown(&world);
            return 1;
        }
    }
    ecs_world_shutdown(&world);
    return 0;
}

static int test_script_apply_large_sketch_within_model_limit(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "LargeSketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) {
        ecs_world_shutdown(&world);
        return 1;
    }

    const int point_count = 140;
    for (int i = 0; i < point_count; i++) {
        float x = (float)i * 0.01f;
        ecs_entity_t p = scene_add_point_to_sketch(&scene,
                                                   sketch,
                                                   vec3_make(x, 0.0f, 0.0f),
                                                   vec4_make(1.0f, 1.0f, 1.0f, 1.0f),
                                                   0.02f);
        if (p == 0) {
            ecs_world_shutdown(&world);
            return 1;
        }
    }
    scene_normalize_sketch_script_local_ids(&scene, sketch);

    char emitted[65536] = {0};
    sketch_script_error_t emit_err = {0};
    if (!scene_script_emit_for_sketch(&scene, sketch, emitted, sizeof(emitted), &emit_err)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    if (strstr(emitted, "return {\n") == NULL || strstr(emitted, "  entities = {\n") == NULL) {
        ecs_world_shutdown(&world);
        return 1;
    }
    size_t emitted_len = strlen(emitted);
    if (emitted_len < 6 || strcmp(emitted + emitted_len - 6, "  }\n}\n") != 0) {
        ecs_world_shutdown(&world);
        return 1;
    }

    sketch_script_error_t preview_err = {0};
    if (!scene_script_preview_parse(&scene, sketch, emitted, &preview_err)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    sketch_script_error_t apply_err = {0};
    if (!scene_script_apply_commit(&scene, sketch, emitted, &apply_err)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    if (scene_count_sketch_geometry(&scene, sketch) != point_count) {
        ecs_world_shutdown(&world);
        return 1;
    }

    char re_emitted[65536] = {0};
    sketch_script_error_t re_emit_err = {0};
    if (!scene_script_emit_for_sketch(&scene, sketch, re_emitted, sizeof(re_emitted), &re_emit_err)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    if (strcmp(emitted, re_emitted) != 0) {
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_world_shutdown(&world);
    return 0;
}

static int test_script_preview_rejects_over_model_limit_without_truncation(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "OverLimitSketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) {
        ecs_world_shutdown(&world);
        return 1;
    }

    const int point_count = SKETCH_SCRIPT_MODEL_MAX_ENTITIES + 8;
    for (int i = 0; i < point_count; i++) {
        float x = (float)i * 0.01f;
        ecs_entity_t p = scene_add_point_to_sketch(&scene,
                                                   sketch,
                                                   vec3_make(x, 0.0f, 0.0f),
                                                   vec4_make(1.0f, 1.0f, 1.0f, 1.0f),
                                                   0.02f);
        if (p == 0) {
            ecs_world_shutdown(&world);
            return 1;
        }
    }
    scene_normalize_sketch_script_local_ids(&scene, sketch);

    char emitted[262144] = {0};
    size_t off = 0;
    int wrote = snprintf(emitted + off, sizeof(emitted) - off, "return {\n  entities = {\n");
    if (wrote <= 0) {
        ecs_world_shutdown(&world);
        return 1;
    }
    off += (size_t)wrote;
    for (int i = 0; i < point_count; i++) {
        wrote = snprintf(emitted + off, sizeof(emitted) - off,
                         "    { id = \"geometry_%d\", type = \"point\", point = {%d, 0, 0} }%s\n",
                         i + 1, i, (i + 1 < point_count) ? "," : "");
        if (wrote <= 0 || off + (size_t)wrote >= sizeof(emitted)) {
            ecs_world_shutdown(&world);
            return 1;
        }
        off += (size_t)wrote;
    }
    wrote = snprintf(emitted + off, sizeof(emitted) - off, "  },\n  constraints = {\n  }\n}");
    if (wrote <= 0 || off + (size_t)wrote >= sizeof(emitted)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    sketch_script_error_t preview_err = {0};
    if (scene_script_preview_parse(&scene, sketch, emitted, &preview_err)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    if (strstr(preview_err.message, "too many entities in script model.") == NULL) {
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_world_shutdown(&world);
    return 0;
}

static int test_script_apply_handles_max_entity_model_without_crash(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "MaxEntitySketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) {
        ecs_world_shutdown(&world);
        return 1;
    }

    const int point_count = SKETCH_SCRIPT_MODEL_MAX_ENTITIES;
    for (int i = 0; i < point_count; i++) {
        float x = (float)i * 0.01f;
        ecs_entity_t p = scene_add_point_to_sketch(&scene,
                                                   sketch,
                                                   vec3_make(x, 0.0f, 0.0f),
                                                   vec4_make(1.0f, 1.0f, 1.0f, 1.0f),
                                                   0.02f);
        if (p == 0) {
            ecs_world_shutdown(&world);
            return 1;
        }
    }
    scene_normalize_sketch_script_local_ids(&scene, sketch);

    char emitted[262144] = {0};
    sketch_script_error_t emit_err = {0};
    if (!scene_script_emit_for_sketch(&scene, sketch, emitted, sizeof(emitted), &emit_err)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    sketch_script_error_t apply_err = {0};
    if (!scene_script_apply_commit(&scene, sketch, emitted, &apply_err)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    if (scene_count_sketch_geometry(&scene, sketch) != point_count) {
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_world_shutdown(&world);
    return 0;
}


typedef int (*script_test_fn_t)(void);

typedef struct script_test_case_t {
    const char *name;
    script_test_fn_t fn;
} script_test_case_t;

int main(void) {
    static const script_test_case_t tests[] = {
        { "test_runtime_rejects_non_54", test_runtime_rejects_non_54 },
        { "test_contract_decl_validation", test_contract_decl_validation },
        { "test_script_identity_roundtrip", test_script_identity_roundtrip },
        { "test_script_apply_reconstructs_supported_scope", test_script_apply_reconstructs_supported_scope },
        { "test_script_apply_resolves_forward_references_two_pass", test_script_apply_resolves_forward_references_two_pass },
        { "test_script_apply_commit_is_atomic_on_unresolved_reference", test_script_apply_commit_is_atomic_on_unresolved_reference },
        { "test_script_preview_parse_preserves_committed_scene_on_failure", test_script_preview_parse_preserves_committed_scene_on_failure },
        { "test_script_preview_parse_rejects_illegal_constraint_participants", test_script_preview_parse_rejects_illegal_constraint_participants },
        { "test_script_roundtrip_parallel_perpendicular_group_constraints_lcon04",
          test_script_roundtrip_parallel_perpendicular_group_constraints_lcon04 },
        { "test_script_parse_rejects_unexpected_tokens_between_blocks", test_script_parse_rejects_unexpected_tokens_between_blocks },
        { "test_script_apply_commit_keeps_last_valid_scene_on_failure", test_script_apply_commit_keeps_last_valid_scene_on_failure },
        { "test_script_apply_undo_redo_single_step", test_script_apply_undo_redo_single_step },
        { "test_script_apply_failure_preserves_last_valid_state", test_script_apply_failure_preserves_last_valid_state },
        { "test_script_apply_respects_undo_suppression_flag", test_script_apply_respects_undo_suppression_flag },
        { "test_script_io_numeric_schema_roundtrip", test_script_io_numeric_schema_roundtrip },
        { "test_script_io_rejects_non_numeric", test_script_io_rejects_non_numeric },
        { "test_script_io_live_edit_uses_transaction_pipeline", test_script_io_live_edit_uses_transaction_pipeline },
        { "test_script_io_live_edit_handles_large_script_buffers", test_script_io_live_edit_handles_large_script_buffers },
        { "test_script_io_numeric_input_coalesces_single_undo_step", test_script_io_numeric_input_coalesces_single_undo_step },
        { "test_script_apply_transaction_uses_transaction_command_type", test_script_apply_transaction_uses_transaction_command_type },
        { "test_script_apply_transaction_noop_does_not_push_undo", test_script_apply_transaction_noop_does_not_push_undo },
        { "test_script_io_window_request_is_exposed_from_inspector_state", test_script_io_window_request_is_exposed_from_inspector_state },
        { "test_script_apply_preserves_labels_by_script_identity", test_script_apply_preserves_labels_by_script_identity },
        { "test_script_editor_launch_request_is_exposed_from_inspector_state", test_script_editor_launch_request_is_exposed_from_inspector_state },
        { "test_geometry_manager_mutations_trigger_dirty_callback", test_geometry_manager_mutations_trigger_dirty_callback },
        { "test_script_emit_orders_by_type_and_script_id", test_script_emit_orders_by_type_and_script_id },
        { "test_script_emit_formats_numbers_without_scientific_notation", test_script_emit_formats_numbers_without_scientific_notation },
        { "test_script_emit_noop_stability", test_script_emit_noop_stability },
        { "test_script_reemit_revision_changes_on_mutation", test_script_reemit_revision_changes_on_mutation },
        { "test_script_emit_includes_incremental_manual_additions_after_script_activation",
          test_script_emit_includes_incremental_manual_additions_after_script_activation },
        { "test_registry_parity_deterministic_contract_error", test_registry_parity_deterministic_contract_error },
        { "test_descriptor_roundtrip_preserves_role_and_sub_index", test_descriptor_roundtrip_preserves_role_and_sub_index },
        { "test_deterministic_contract_error_atomic_reject", test_deterministic_contract_error_atomic_reject },
        { "test_color_roundtrip_and_non_script_color_untouched", test_color_roundtrip_and_non_script_color_untouched },
        { "test_repeat_apply_deterministic", test_repeat_apply_deterministic },
        { "test_script_apply_large_sketch_within_model_limit",
          test_script_apply_large_sketch_within_model_limit },
        { "test_script_apply_handles_max_entity_model_without_crash",
          test_script_apply_handles_max_entity_model_without_crash },
        { "test_script_preview_rejects_over_model_limit_without_truncation",
          test_script_preview_rejects_over_model_limit_without_truncation }
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
