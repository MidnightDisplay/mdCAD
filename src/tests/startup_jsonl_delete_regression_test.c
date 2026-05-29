#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include "../startup_jsonl_import_controller.h"
#include "../jsonl_observer_system.h"
#include "../selection.h"
#include "../undo_redo_exec.h"
#include "../ui/ui_scene_hierarchy.h"

#include "win32_embed_test_stub.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
void sokol_main(void) {}
#ifdef __cplusplus
}
#endif

static int make_absolute_path(const char *relative_path, char *buffer, size_t buffer_size) {
    if (!relative_path || !buffer || buffer_size == 0u) {
        return 0;
    }
#if defined(_WIN32)
    return _fullpath(buffer, relative_path, buffer_size) != NULL;
#else
    char cwd[1024];
    if (!getcwd(cwd, sizeof(cwd))) {
        return 0;
    }
    snprintf(buffer, buffer_size, "%s/%s", cwd, relative_path);
    return 1;
#endif
}

static int write_startup_jsonl_fixture(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    int ok = fputs("{\"Name\":\"Entry\",\"Elements\":[]}\n", f) >= 0;
    fclose(f);
    return ok;
}

static int run_startup_import(ecs_scene_t *scene,
                              const char *absolute_fixture,
                              int live_refresh_enabled,
                              startup_jsonl_import_controller_t *controller) {
    if (!scene || !absolute_fixture || !controller) return 0;
    startup_jsonl_import_controller_init(controller);
    if (!startup_jsonl_import_controller_arm(controller, absolute_fixture, live_refresh_enabled != 0)) {
        return 0;
    }
    if (!startup_jsonl_import_controller_tick(controller, scene)) {
        return controller->state == STARTUP_JSONL_IMPORT_COMPLETE;
    }
    return controller->state == STARTUP_JSONL_IMPORT_COMPLETE;
}

static ecs_entity_t find_first_imported_child_under_root(ecs_scene_t *scene, ecs_entity_t root) {
    if (!scene || root == 0) return 0;
    int root_child_count = scene_count_children(scene, root);
    if (root_child_count <= 0) return 0;

    ecs_entity_t *root_children = (ecs_entity_t *)malloc(sizeof(ecs_entity_t) * (size_t)root_child_count);
    if (!root_children) return 0;

    int root_collected = scene_get_children(scene, root, root_children, root_child_count);
    ecs_entity_t found = (root_collected > 0) ? root_children[0] : 0;
    free(root_children);
    return found;
}

static ecs_entity_t ensure_imported_child_under_root(ecs_scene_t *scene, ecs_entity_t root) {
    ecs_entity_t child = find_first_imported_child_under_root(scene, root);
    if (child != 0) {
        return child;
    }
    child = scene_add_anchor(scene, "Imported Child", "startup-imported delete fixture child");
    if (child != 0) {
        scene_set_parent(scene, child, root);
    }
    return child;
}

static int find_entity_index(const uint64_t *entity_ids, int count, ecs_entity_t target) {
    if (!entity_ids || count <= 0 || target == 0) return -1;
    for (int i = 0; i < count; i++) {
        if ((ecs_entity_t)entity_ids[i] == target) {
            return i;
        }
    }
    return -1;
}

static int test_startup_import_child_delete_keeps_root_stable(int live_refresh_enabled) {
    const char *relative_fixture = live_refresh_enabled
        ? "startup_jsonl_delete_child_live.jsonl"
        : "startup_jsonl_delete_child_plain.jsonl";
    char absolute_fixture[MDCAD_STARTUP_JSONL_PATH_MAX] = {0};
    if (!write_startup_jsonl_fixture(relative_fixture)) return 1;
    if (!make_absolute_path(relative_fixture, absolute_fixture, sizeof(absolute_fixture))) {
        remove(relative_fixture);
        return 1;
    }

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    selection_buffer_t selection = {0};
    undo_redo_t undo = {0};
    ui_scene_hierarchy_state_t hierarchy = {0};
    startup_jsonl_import_controller_t controller = {0};
    int failed = 0;

    ecs_world_init(&world);
    memset(&scene, 0, sizeof(scene));
    scene.world = &world;
    scene.visible = true;
    selection_init(&selection, &world);
    undo_redo_init(&undo, &scene, 16);
    undo_redo_set_selection(&undo, &selection);
    ui_scene_hierarchy_init(&hierarchy, &selection, &scene);
    ui_scene_hierarchy_set_undo_redo(&hierarchy, &undo);

    if (!run_startup_import(&scene, absolute_fixture, live_refresh_enabled, &controller)) {
        fprintf(stderr, "  fail: startup import did not complete live_refresh=%d\n", live_refresh_enabled);
        failed = 1;
    }

    ecs_entity_t root = controller.job.root_entity;
    ecs_entity_t child = failed ? 0 : ensure_imported_child_under_root(&scene, root);
    if (!failed && (root == 0 || child == 0)) {
        fprintf(stderr, "  fail: expected startup import root/child root=%llu child=%llu\n",
                (unsigned long long)root, (unsigned long long)child);
        failed = 1;
    }

    if (!failed) {
        selection_set_single(&selection, child);
        ui_scene_hierarchy_delete_entities(&hierarchy, &child, 1);
        if (ecs_is_alive(world.world, child)) {
            fprintf(stderr, "  fail: imported child stayed alive after delete child=%llu\n",
                    (unsigned long long)child);
            failed = 1;
        }
        if (!ecs_is_alive(world.world, root)) {
            fprintf(stderr, "  fail: imported root died after child delete root=%llu\n",
                    (unsigned long long)root);
            failed = 1;
        }
        if (selection_count(&selection) != 0) {
            fprintf(stderr, "  fail: selection not cleared after child delete count=%d selected=%llu\n",
                    selection_count(&selection),
                    (unsigned long long)selection_get(&selection, 0));
            failed = 1;
        }
    }

    ui_scene_hierarchy_shutdown(&hierarchy);
    undo_redo_shutdown(&undo);
    selection_shutdown(&selection);
    ecs_world_shutdown(&world);
    remove(relative_fixture);
    return failed;
}

static int test_startup_import_bulk_delete_canonicalizes_root_order(void) {
    const char *relative_fixture = "startup_jsonl_delete_bulk_canonicalization.jsonl";
    char absolute_fixture[MDCAD_STARTUP_JSONL_PATH_MAX] = {0};
    if (!write_startup_jsonl_fixture(relative_fixture)) return 1;
    if (!make_absolute_path(relative_fixture, absolute_fixture, sizeof(absolute_fixture))) {
        remove(relative_fixture);
        return 1;
    }

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    selection_buffer_t selection = {0};
    undo_redo_t undo = {0};
    ui_scene_hierarchy_state_t hierarchy = {0};
    startup_jsonl_import_controller_t controller = {0};
    int failed = 0;

    ecs_world_init(&world);
    memset(&scene, 0, sizeof(scene));
    scene.world = &world;
    scene.visible = true;
    selection_init(&selection, &world);
    undo_redo_init(&undo, &scene, 16);
    undo_redo_set_selection(&undo, &selection);
    ui_scene_hierarchy_init(&hierarchy, &selection, &scene);
    ui_scene_hierarchy_set_undo_redo(&hierarchy, &undo);

    if (!run_startup_import(&scene, absolute_fixture, 1, &controller)) {
        fprintf(stderr, "  fail: startup import did not complete for canonicalization test\n");
        failed = 1;
    }

    ecs_entity_t root = controller.job.root_entity;
    ecs_entity_t child = failed ? 0 : ensure_imported_child_under_root(&scene, root);
    if (!failed && (root == 0 || child == 0)) {
        fprintf(stderr, "  fail: expected root/child for canonicalization root=%llu child=%llu\n",
                (unsigned long long)root, (unsigned long long)child);
        failed = 1;
    }

    if (!failed) {
        selection_add(&selection, child);
        selection_add(&selection, root);

        ecs_entity_t to_delete[2] = { child, root };
        ui_scene_hierarchy_delete_entities(&hierarchy, to_delete, 2);

        if (undo.count != 1 || undo.current != 1 || undo.commands[0].type != CMD_BULK_DELETE_ENTITIES) {
            fprintf(stderr, "  fail: undo stack mismatch count=%d current=%d type=%d\n",
                    undo.count, undo.current, (undo.count > 0) ? (int)undo.commands[0].type : -1);
            failed = 1;
        } else {
            const cmd_bulk_delete_entities_t *bulk = &undo.commands[0].data.bulk_delete;
            int root_index = find_entity_index(bulk->entity_ids, bulk->count, root);
            int child_index = find_entity_index(bulk->entity_ids, bulk->count, child);
            if (root_index < 0 || child_index < 0 || root_index >= child_index) {
                fprintf(stderr, "  fail: expected root to precede child in undo snapshot root_index=%d child_index=%d count=%d\n",
                        root_index, child_index, bulk->count);
                failed = 1;
            }
        }
    }

    ui_scene_hierarchy_shutdown(&hierarchy);
    undo_redo_shutdown(&undo);
    selection_shutdown(&selection);
    ecs_world_shutdown(&world);
    remove(relative_fixture);
    return failed;
}

static int test_startup_import_root_delete_cancels_active_refresh(void) {
    const char *relative_fixture = "startup_jsonl_delete_root_live_refresh.jsonl";
    char absolute_fixture[MDCAD_STARTUP_JSONL_PATH_MAX] = {0};
    if (!write_startup_jsonl_fixture(relative_fixture)) return 1;
    if (!make_absolute_path(relative_fixture, absolute_fixture, sizeof(absolute_fixture))) {
        remove(relative_fixture);
        return 1;
    }

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    selection_buffer_t selection = {0};
    undo_redo_t undo = {0};
    ui_scene_hierarchy_state_t hierarchy = {0};
    startup_jsonl_import_controller_t controller = {0};
    int failed = 0;

    ecs_world_init(&world);
    memset(&scene, 0, sizeof(scene));
    scene.world = &world;
    scene.visible = true;
    selection_init(&selection, &world);
    undo_redo_init(&undo, &scene, 16);
    undo_redo_set_selection(&undo, &selection);
    ui_scene_hierarchy_init(&hierarchy, &selection, &scene);
    ui_scene_hierarchy_set_undo_redo(&hierarchy, &undo);

    if (!run_startup_import(&scene, absolute_fixture, 1, &controller)) {
        fprintf(stderr, "  fail: startup import did not complete for root delete test\n");
        failed = 1;
    }

    ecs_entity_t root = controller.job.root_entity;
    ecs_entity_t child = failed ? 0 : ensure_imported_child_under_root(&scene, root);
    if (!failed && (root == 0 || child == 0)) {
        fprintf(stderr, "  fail: expected root/child for root delete test root=%llu child=%llu\n",
                (unsigned long long)root, (unsigned long long)child);
        failed = 1;
    }

    if (!failed && !jsonl_observer_request_flat_refresh(&scene, root, &selection)) {
        fprintf(stderr, "  fail: live refresh request did not start before root delete\n");
        failed = 1;
    }
    if (!failed && !jsonl_observer_is_flat_refresh_running(&scene, root)) {
        fprintf(stderr, "  fail: live refresh slot not active before root delete\n");
        failed = 1;
    }

    if (!failed) {
        selection_add(&selection, child);
        selection_add(&selection, root);
        ui_scene_hierarchy_delete_entities(&hierarchy, &root, 1);

        if (ecs_is_alive(world.world, root)) {
            fprintf(stderr, "  fail: root stayed alive after root delete root=%llu\n",
                    (unsigned long long)root);
            failed = 1;
        }
        if (ecs_is_alive(world.world, child)) {
            fprintf(stderr, "  fail: descendant child stayed alive after root delete child=%llu\n",
                    (unsigned long long)child);
            failed = 1;
        }
        if (selection_count(&selection) != 0) {
            fprintf(stderr, "  fail: selection not cleared after root delete count=%d selected=%llu\n",
                    selection_count(&selection),
                    (unsigned long long)selection_get(&selection, 0));
            failed = 1;
        }
        if (jsonl_observer_is_flat_refresh_running(&scene, root)) {
            fprintf(stderr, "  fail: live refresh slot stayed active after root delete root=%llu\n",
                    (unsigned long long)root);
            failed = 1;
        }
    }

    ui_scene_hierarchy_shutdown(&hierarchy);
    undo_redo_shutdown(&undo);
    selection_shutdown(&selection);
    ecs_world_shutdown(&world);
    remove(relative_fixture);
    return failed;
}

typedef int (*test_fn_t)(void);
typedef struct {
    const char *name;
    test_fn_t fn;
} test_case_t;

static int test_startup_import_child_delete_plain(void) {
    return test_startup_import_child_delete_keeps_root_stable(0);
}

static int test_startup_import_child_delete_live_refresh(void) {
    return test_startup_import_child_delete_keeps_root_stable(1);
}

int main(void) {
    static const test_case_t tests[] = {
        { "test_startup_import_child_delete_plain", test_startup_import_child_delete_plain },
        { "test_startup_import_child_delete_live_refresh", test_startup_import_child_delete_live_refresh },
        { "test_startup_import_bulk_delete_canonicalizes_root_order", test_startup_import_bulk_delete_canonicalizes_root_order },
        { "test_startup_import_root_delete_cancels_active_refresh", test_startup_import_root_delete_cancels_active_refresh },
    };

    stm_setup();

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }

    return 0;
}
