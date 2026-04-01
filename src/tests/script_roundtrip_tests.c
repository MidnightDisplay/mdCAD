#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include "../scene_serializer.h"
#include "../scripting/sketch_script_runtime.h"
#include "../scripting/sketch_script_contract.h"
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

int main(void) {
    if (test_runtime_rejects_non_54() != 0) return 1;
    if (test_contract_decl_validation() != 0) return 1;
    if (test_script_identity_roundtrip() != 0) return 1;
    return 0;
}
