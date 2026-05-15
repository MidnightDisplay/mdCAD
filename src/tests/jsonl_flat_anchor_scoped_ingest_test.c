#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include "../jsonl_import_job.h"
#include "win32_embed_test_stub.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
void sokol_main(void) {}
#ifdef __cplusplus
}
#endif

static bool write_jsonl_fixture_lines(const char *path, const char *const *lines, int line_count) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    for (int i = 0; i < line_count; i++) {
        if (fputs(lines[i], f) < 0 || fputs("\n", f) < 0) {
            fclose(f);
            return false;
        }
    }
    fclose(f);
    return true;
}

static bool run_flat_import_to_completion(ecs_scene_t *scene, const char *path, jsonl_import_job_t *out_job) {
    jsonl_import_job_t job = {0};
    jsonl_import_job_init(&job);
    jsonl_import_job_set_mesh_mode(&job, 0);
    if (!jsonl_import_job_start(&job, path, 1.0f, true, vec4_make(1, 1, 1, 1), false, 0.0f, 0.0f, 0.0f)) {
        return false;
    }

    int guard = 10000;
    while (!jsonl_import_job_tick(&job, scene)) {
        if (--guard <= 0) {
            jsonl_import_job_cancel(&job, scene);
            return false;
        }
    }

    if (job.state != JSONL_JOB_COMPLETE) {
        return false;
    }

    if (out_job) *out_job = job;
    return true;
}

static int collect_children(ecs_world_state_t *w, ecs_entity_t parent, ecs_entity_t *out, int max_count) {
    if (!w || !parent || !out || max_count <= 0) return 0;
    return ecs_world_get_children(w, parent, out, max_count);
}

static ecs_entity_t find_child_with_label(ecs_world_state_t *w,
                                          ecs_entity_t parent,
                                          const char *name,
                                          const char *description) {
    ecs_entity_t children[256] = {0};
    int child_count = collect_children(w, parent, children, 256);
    for (int i = 0; i < child_count; i++) {
        LabelComp *label = ecs_world_get_label(w, children[i]);
        if (!label) continue;
        if (name && strcmp(label->name, name) != 0) continue;
        if (description && strcmp(label->description, description) != 0) continue;
        return children[i];
    }
    return 0;
}

static int assert_root_label_matches(ecs_world_state_t *w,
                                     ecs_entity_t root,
                                     const char *expected_name,
                                     const char *expected_desc) {
    LabelComp *label = ecs_world_get_label(w, root);
    if (!label) return 1;
    if (strcmp(label->name, expected_name) != 0) return 1;
    if (strcmp(label->description, expected_desc) != 0) return 1;
    return 0;
}

static int test_flat_import_creates_root_entry_geometry_hierarchy(void) {
    const char *fixture = "jsonl_flat_anchor_hierarchy.jsonl";
    const char *lines[] = {
        "{\"Name\":\"EntryA\",\"Description\":\"EntryADesc\",\"Elements\":[{\"Name\":\"P\",\"Description\":\"PointDesc\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":1,\"Y\":2,\"Z\":3}}]}"
    };
    if (!write_jsonl_fixture_lines(fixture, lines, 1)) return 1;

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    jsonl_import_job_t job = {0};
    int failed = 0;

    if (!run_flat_import_to_completion(&scene, fixture, &job)) failed = 1;
    if (!failed && job.root_entity == 0) failed = 1;
    if (!failed && scene_is_sketch(&scene, job.root_entity)) failed = 1;

    ecs_entity_t root_children[16] = {0};
    int root_child_count = 0;
    if (!failed) {
        root_child_count = collect_children(&world, job.root_entity, root_children, 16);
        if (root_child_count != 1) failed = 1;
    }

    ecs_entity_t entry = 0;
    if (!failed) {
        entry = root_children[0];
        if (!entry || scene_is_sketch(&scene, entry)) failed = 1;
        if (ecs_world_get_parent(&world, entry) != job.root_entity) failed = 1;
    }

    ecs_entity_t entry_children[16] = {0};
    int entry_child_count = 0;
    if (!failed) {
        entry_child_count = collect_children(&world, entry, entry_children, 16);
        if (entry_child_count != 1) failed = 1;
    }

    if (!failed) {
        ecs_entity_t geom = entry_children[0];
        if (!geom) failed = 1;
        if (!ecs_world_get_geometry(&world, geom)) failed = 1;
        if (scene_is_sketch(&scene, geom)) failed = 1;
        if (ecs_world_get_parent(&world, geom) != entry) failed = 1;
    }

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(fixture);
    return failed;
}

static int test_flat_import_skips_empty_entries(void) {
    const char *fixture = "jsonl_flat_anchor_skip_empty.jsonl";
    const char *lines[] = {
        "{\"Name\":\"EmptyEntry\",\"Description\":\"No geometry\",\"Elements\":[]}",
        "{\"Name\":\"WithGeom\",\"Description\":\"Has geometry\",\"Elements\":[{\"Name\":\"P\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":4,\"Y\":5,\"Z\":6}}]}"
    };
    if (!write_jsonl_fixture_lines(fixture, lines, 2)) return 1;

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    jsonl_import_job_t job = {0};
    int failed = 0;

    if (!run_flat_import_to_completion(&scene, fixture, &job)) failed = 1;
    if (!failed) {
        ecs_entity_t root_children[32] = {0};
        int count = collect_children(&world, job.root_entity, root_children, 32);
        if (count != 1) failed = 1;
        if (!failed) {
            ecs_entity_t non_empty = find_child_with_label(&world, job.root_entity, "WithGeom", "Has geometry");
            ecs_entity_t empty = find_child_with_label(&world, job.root_entity, "EmptyEntry", "No geometry");
            if (non_empty == 0 || empty != 0) failed = 1;
        }
    }

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(fixture);
    return failed;
}

static int test_flat_import_applies_root_and_entry_label_contracts(void) {
    const char *fixture = "jsonl_flat_anchor_label_contract.jsonl";
    const char *lines[] = {
        "{\"Name\":\"EntryLabel\",\"Description\":\"EntryDescription\",\"Elements\":[{\"Name\":\"L\",\"Description\":\"Line\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Line3D\",\"StartPoint\":{\"X\":0,\"Y\":0,\"Z\":0},\"EndPoint\":{\"X\":1,\"Y\":0,\"Z\":0}}}]}"
    };
    if (!write_jsonl_fixture_lines(fixture, lines, 1)) return 1;

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    jsonl_import_job_t job = {0};
    int failed = 0;

    if (!run_flat_import_to_completion(&scene, fixture, &job)) failed = 1;
    if (!failed && assert_root_label_matches(&world, job.root_entity, "jsonl_flat_anchor_label_contract", fixture) != 0) {
        failed = 1;
    }

    if (!failed) {
        ecs_entity_t entry = find_child_with_label(&world, job.root_entity, "EntryLabel", "EntryDescription");
        if (entry == 0) failed = 1;
    }

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(fixture);
    return failed;
}

static int test_flat_import_reimport_creates_suffixed_new_root(void) {
    const char *fixture = "jsonl_flat_anchor_reimport.jsonl";
    const char *lines[] = {
        "{\"Name\":\"Entry\",\"Description\":\"Desc\",\"Elements\":[{\"Name\":\"P\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":7,\"Y\":8,\"Z\":9}}]}"
    };
    if (!write_jsonl_fixture_lines(fixture, lines, 1)) return 1;

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    jsonl_import_job_t first = {0};
    jsonl_import_job_t second = {0};
    int failed = 0;

    if (!run_flat_import_to_completion(&scene, fixture, &first)) failed = 1;
    if (!failed && !run_flat_import_to_completion(&scene, fixture, &second)) failed = 1;
    if (!failed && (first.root_entity == 0 || second.root_entity == 0 || first.root_entity == second.root_entity)) failed = 1;

    if (!failed && assert_root_label_matches(&world, first.root_entity, "jsonl_flat_anchor_reimport", fixture) != 0) {
        failed = 1;
    }
    if (!failed && assert_root_label_matches(&world, second.root_entity, "jsonl_flat_anchor_reimport (2)", fixture) != 0) {
        failed = 1;
    }

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(fixture);
    return failed;
}

static int test_flat_import_preserves_existing_selection(void) {
    const char *fixture = "jsonl_flat_anchor_selection.jsonl";
    const char *lines[] = {
        "{\"Name\":\"Entry\",\"Description\":\"Desc\",\"Elements\":[{\"Name\":\"P\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":10,\"Y\":11,\"Z\":12}}]}"
    };
    if (!write_jsonl_fixture_lines(fixture, lines, 1)) return 1;

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t selected = scene_add_point(&scene, mdcad_import_vec3_make(0, 0, 0), vec4_make(1, 0, 0, 1), 0.05f);
    if (selected == 0) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        remove(fixture);
        return 1;
    }
    ecs_world_select(&world, selected);

    jsonl_import_job_t job = {0};
    int failed = 0;
    if (!run_flat_import_to_completion(&scene, fixture, &job)) failed = 1;

    if (!failed && !ecs_world_is_selected(&world, selected)) failed = 1;
    if (!failed && ecs_world_is_selected(&world, job.root_entity)) failed = 1;

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(fixture);
    return failed;
}

typedef int (*test_fn_t)(void);
typedef struct {
    const char *name;
    test_fn_t fn;
} test_case_t;

int main(void) {
    stm_setup();

    static const test_case_t tests[] = {
        { "test_flat_import_creates_root_entry_geometry_hierarchy", test_flat_import_creates_root_entry_geometry_hierarchy },
        { "test_flat_import_skips_empty_entries", test_flat_import_skips_empty_entries },
        { "test_flat_import_applies_root_and_entry_label_contracts", test_flat_import_applies_root_and_entry_label_contracts },
        { "test_flat_import_reimport_creates_suffixed_new_root", test_flat_import_reimport_creates_suffixed_new_root },
        { "test_flat_import_preserves_existing_selection", test_flat_import_preserves_existing_selection },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }

    return 0;
}
