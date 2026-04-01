#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include "../scene_serializer.h"
#include "../scripting/sketch_script_runtime.h"
#include "../scripting/sketch_script_contract.h"
#include "../scripting/sketch_script_parse.h"
#include "../scripting/sketch_script_apply.h"
#include "../scripting/sketch_script_emit.h"
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
    if (test_script_emit_orders_by_type_and_script_id() != 0) return 1;
    if (test_script_emit_formats_numbers_without_scientific_notation() != 0) return 1;
    if (test_script_emit_noop_stability() != 0) return 1;
    if (test_script_reemit_revision_changes_on_mutation() != 0) return 1;
    return 0;
}
