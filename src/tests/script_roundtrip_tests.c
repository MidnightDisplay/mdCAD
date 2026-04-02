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
        "    { id = \"constraint_1\", type = \"Coincident\", participants = {\"geometry_1\", \"geometry_2\"} },\n"
        "    { id = \"constraint_2\", type = \"Length\", participants = {\"geometry_2\"}, value = 5.0, driven = false }\n"
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
        "    { id = \"constraint_1\", type = \"Coincident\", participants = {\"geometry_1\", \"geometry_2\"} }\n"
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
        "    { id = \"constraint_1\", type = \"Coincident\", participants = {\"geometry_1\", \"missing_geometry\"} }\n"
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
        "    { id = \"constraint_1\", type = \"Coincident\", participants = {\"geometry_1\", \"geometry_2\"} }\n"
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
        "    { id = \"constraint_1\", type = \"Coincident\", participants = {\"geometry_1\", \"missing\"} }\n"
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
        "    { id = \"constraint_1\", type = \"Perpendicular\", participants = {\"geometry_1\", \"geometry_3\"} }\n"
        "  }\n"
        "}";
    sketch_script_error_t error = {0};
    bool ok = scene_script_preview_parse(&scene, sketch, invalid_script, &error);
    ecs_world_shutdown(&world);
    if (ok) return 1;
    return error.message[0] != '\0' ? 0 : 1;
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
        "    { id = \"constraint_1\", type = \"Fixed\", participants = {\"geometry_1\"} } xyz\n"
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
        "    { id = \"constraint_1\", type = \"Coincident\", participants = {\"geometry_1\", \"missing_geometry\"} }\n"
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
        "    { id = \"constraint_1\", type = \"Coincident\", participants = {\"geometry_1\", \"geometry_2\"} }\n"
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
        "    { id = \"constraint_1\", type = \"Coincident\", participants = {\"geometry_1\", \"geometry_2\"} }\n"
        "  }\n"
        "}";
    const char *invalid_script =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"point\", point = {0, 0, 0} },\n"
        "    { id = \"geometry_2\", type = \"line\", a = {0, 0, 0}, b = {1, 0, 0} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_1\", type = \"Coincident\", participants = {\"geometry_1\", \"missing_geometry\"} }\n"
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

static int test_scene_script_io_apply_uses_transactional_apply(void) {
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

    int undo_before = undo_redo_get_undo_count(&undo_redo);
    if (!scene_script_io_apply_input_value(&scene, sketch, "input_length", 12.5, &err)) {
        undo_redo_shutdown(&undo_redo);
        ecs_world_shutdown(&world);
        return 1;
    }
    int undo_after = undo_redo_get_undo_count(&undo_redo);
    undo_redo_shutdown(&undo_redo);
    ecs_world_shutdown(&world);
    return (undo_after - undo_before) == 1 ? 0 : 1;
}

static int test_script_editor_launch_request_is_exposed_from_inspector_state(void) {
    selection_buffer_t selection = {0};
    ecs_world_state_t world = {0};
    ui_entity_inspector_state_t inspector = {0};
    ui_entity_inspector_init(&inspector, &selection, &world);

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


int main(void) {
    if (test_runtime_rejects_non_54() != 0) return 1;
    if (test_contract_decl_validation() != 0) return 1;
    if (test_script_identity_roundtrip() != 0) return 1;
    if (test_script_apply_reconstructs_supported_scope() != 0) return 1;
    if (test_script_apply_resolves_forward_references_two_pass() != 0) return 1;
    if (test_script_apply_commit_is_atomic_on_unresolved_reference() != 0) return 1;
    if (test_script_preview_parse_preserves_committed_scene_on_failure() != 0) return 1;
    if (test_script_preview_parse_rejects_illegal_constraint_participants() != 0) return 1;
    if (test_script_parse_rejects_unexpected_tokens_between_blocks() != 0) return 1;
    if (test_script_apply_commit_keeps_last_valid_scene_on_failure() != 0) return 1;
    if (test_script_apply_undo_redo_single_step() != 0) return 1;
    if (test_script_apply_failure_preserves_last_valid_state() != 0) return 1;
    if (test_script_io_numeric_schema_roundtrip() != 0) return 1;
    if (test_script_io_rejects_non_numeric() != 0) return 1;
    if (test_scene_script_io_apply_uses_transactional_apply() != 0) return 1;
    if (test_script_editor_launch_request_is_exposed_from_inspector_state() != 0) return 1;
    if (test_script_emit_orders_by_type_and_script_id() != 0) return 1;
    if (test_script_emit_formats_numbers_without_scientific_notation() != 0) return 1;
    if (test_script_emit_noop_stability() != 0) return 1;
    if (test_script_reemit_revision_changes_on_mutation() != 0) return 1;
    return 0;
}
