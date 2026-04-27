set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(SOURCES
    app.c
)

set(MDCAD_NATIVE_DESKTOP OFF)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS" AND NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(MDCAD_NATIVE_DESKTOP ON)
endif()

# Add platform-specific pick buffer readback implementations
if(APPLE)
    list(APPEND SOURCES gpu/pick_readback_metal.m)
elseif(WIN32)
    if(MINGW OR USE_VULKAN)
        # Vulkan readback for MinGW or MSVC with USE_VULKAN=ON
        list(APPEND SOURCES gpu/pick_readback_vulkan.c)
    else()
        # D3D11 readback for MSVC builds (default)
        list(APPEND SOURCES gpu/pick_readback_d3d11.c)
    endif()
endif()

# iOS requires MACOSX_BUNDLE for app packaging
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(IOS_RESOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../ios/LaunchScreen.storyboard
    )
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES} ${IOS_RESOURCES})
    # Compile as Objective-C for UIKit/Foundation access
    target_compile_options(${PROJECT_NAME} PRIVATE -x objective-c)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../ios/Info.plist
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.skl.tmp"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"  # iPhone and iPad
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "14.0"
        RESOURCE "${IOS_RESOURCES}"
        LINKER_LANGUAGE OBJC
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    # Android requires shared library for NativeActivity
    add_library(${PROJECT_NAME} SHARED ${SOURCES})
    # Force ANativeActivity_onCreate to be exported
    target_link_options(${PROJECT_NAME} PRIVATE -u ANativeActivity_onCreate)
    # 16KB page alignment for Android 15+ compatibility
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,-z,max-page-size=16384)
elseif(WIN32)
    # WIN32 flag tells VS to use WinMain entry point (provided by Sokol)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
)

target_link_libraries(${PROJECT_NAME} libsokol libcimgui flecs_static cjson)

if(MDCAD_NATIVE_DESKTOP)
    add_executable(mdcad_math_harness math_harness.c)
    target_compile_features(mdcad_math_harness PRIVATE c_std_11)
    get_target_property(MDCAD_LIBSOKOL_INCLUDE_DIRS libsokol INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(MDCAD_LIBCIMGUI_INCLUDE_DIRS libcimgui INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(mdcad_math_harness PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    if(UNIX AND NOT APPLE)
        target_link_libraries(mdcad_math_harness PRIVATE m)
    endif()

    add_custom_target(math-regression
        COMMAND $<TARGET_FILE:mdcad_math_harness> --mode compare --strict
        DEPENDS mdcad_math_harness
        USES_TERMINAL
    )

    add_custom_target(math-bench
        COMMAND $<TARGET_FILE:mdcad_math_harness> --mode bench --iterations 20000
        DEPENDS mdcad_math_harness
        USES_TERMINAL
    )

    add_custom_target(math-validation
        DEPENDS math-regression math-bench ${PROJECT_NAME}
    )

    include(CTest)
    add_executable(script_roundtrip_tests tests/script_roundtrip_tests.c)
    target_compile_features(script_roundtrip_tests PRIVATE c_std_11)
    get_target_property(MDCAD_LIBSOKOL_INCLUDE_DIRS libsokol INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(MDCAD_LIBCIMGUI_INCLUDE_DIRS libcimgui INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(script_roundtrip_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(script_roundtrip_tests PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME script_roundtrip_tests COMMAND script_roundtrip_tests)

    add_executable(scene_solver_contract tests/scene_solver_contract_test.c)
    target_compile_features(scene_solver_contract PRIVATE c_std_11)
    target_include_directories(scene_solver_contract PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(scene_solver_contract PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME scene_solver_contract COMMAND scene_solver_contract)

    add_executable(scene_solver_drag tests/scene_solver_drag_test.c)
    target_compile_features(scene_solver_drag PRIVATE c_std_11)
    target_include_directories(scene_solver_drag PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(scene_solver_drag PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME scene_solver_drag COMMAND scene_solver_drag)

    add_executable(endpoint_pick tests/endpoint_pick_test.c)
    target_compile_features(endpoint_pick PRIVATE c_std_11)
    target_include_directories(endpoint_pick PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(endpoint_pick PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME endpoint_pick COMMAND endpoint_pick)

    add_executable(scene_solver_diagnostics tests/scene_solver_diagnostics_test.c)
    target_compile_features(scene_solver_diagnostics PRIVATE c_std_11)
    target_include_directories(scene_solver_diagnostics PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(scene_solver_diagnostics PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME scene_solver_diagnostics COMMAND scene_solver_diagnostics)

    add_executable(scene_solver_trigger tests/scene_solver_trigger_test.c)
    target_compile_features(scene_solver_trigger PRIVATE c_std_11)
    target_include_directories(scene_solver_trigger PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(scene_solver_trigger PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME scene_solver_trigger COMMAND scene_solver_trigger)

    add_executable(scene_solver_pass_policy tests/scene_solver_pass_policy_test.c)
    target_compile_features(scene_solver_pass_policy PRIVATE c_std_11)
    target_include_directories(scene_solver_pass_policy PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(scene_solver_pass_policy PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME scene_solver_pass_policy COMMAND scene_solver_pass_policy)

    add_executable(jsonl_sketch_import_test tests/jsonl_sketch_import_test.c)
    target_compile_features(jsonl_sketch_import_test PRIVATE c_std_11)
    target_include_directories(jsonl_sketch_import_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(jsonl_sketch_import_test PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME jsonl_sketch_import_test COMMAND jsonl_sketch_import_test)

    add_executable(jsonl_sketch_mapping_test tests/jsonl_sketch_mapping_test.c)
    target_compile_features(jsonl_sketch_mapping_test PRIVATE c_std_11)
    target_include_directories(jsonl_sketch_mapping_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(jsonl_sketch_mapping_test PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME jsonl_sketch_mapping_test COMMAND jsonl_sketch_mapping_test)

    add_executable(jsonl_observer_state_test tests/jsonl_observer_state_test.c)
    target_compile_features(jsonl_observer_state_test PRIVATE c_std_11)
    target_include_directories(jsonl_observer_state_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(jsonl_observer_state_test PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME jsonl_observer_state_test COMMAND jsonl_observer_state_test)

    add_executable(jsonl_reparse_transaction_test tests/jsonl_reparse_transaction_test.c)
    target_compile_features(jsonl_reparse_transaction_test PRIVATE c_std_11)
    target_include_directories(jsonl_reparse_transaction_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(jsonl_reparse_transaction_test PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME jsonl_reparse_transaction_test COMMAND jsonl_reparse_transaction_test)

    add_executable(jsonl_label_contract_test tests/jsonl_label_contract_test.c)
    target_compile_features(jsonl_label_contract_test PRIVATE c_std_11)
    target_include_directories(jsonl_label_contract_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(jsonl_label_contract_test PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME jsonl_label_contract_test COMMAND jsonl_label_contract_test)

    add_executable(jsonl_loader_limits_test tests/jsonl_loader_limits_test.c)
    target_compile_features(jsonl_loader_limits_test PRIVATE c_std_11)
    target_include_directories(jsonl_loader_limits_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(jsonl_loader_limits_test PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME jsonl_loader_limits_test COMMAND jsonl_loader_limits_test)

    add_executable(jsonl_flat_import_ui_contract_test tests/jsonl_flat_import_ui_contract_test.c)
    target_compile_features(jsonl_flat_import_ui_contract_test PRIVATE c_std_11)
    target_include_directories(jsonl_flat_import_ui_contract_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(jsonl_flat_import_ui_contract_test PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME jsonl_flat_import_ui_contract_test COMMAND jsonl_flat_import_ui_contract_test)

    add_executable(jsonl_flat_import_options_contract_test tests/jsonl_flat_import_options_contract_test.c)
    target_compile_features(jsonl_flat_import_options_contract_test PRIVATE c_std_11)
    target_include_directories(jsonl_flat_import_options_contract_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(jsonl_flat_import_options_contract_test PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME jsonl_flat_import_options_contract_test COMMAND jsonl_flat_import_options_contract_test)

    add_executable(jsonl_flat_anchor_scoped_ingest_test tests/jsonl_flat_anchor_scoped_ingest_test.c)
    target_compile_features(jsonl_flat_anchor_scoped_ingest_test PRIVATE c_std_11)
    target_include_directories(jsonl_flat_anchor_scoped_ingest_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(jsonl_flat_anchor_scoped_ingest_test PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME jsonl_flat_anchor_scoped_ingest_test COMMAND jsonl_flat_anchor_scoped_ingest_test)

    add_executable(jsonl_flat_observer_manual_refresh_test tests/jsonl_flat_observer_manual_refresh_test.c)
    target_compile_features(jsonl_flat_observer_manual_refresh_test PRIVATE c_std_11)
    target_include_directories(jsonl_flat_observer_manual_refresh_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(jsonl_flat_observer_manual_refresh_test PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME jsonl_flat_observer_manual_refresh_test COMMAND jsonl_flat_observer_manual_refresh_test)

    add_executable(jsonl_flat_observer_inspector_contract_test tests/jsonl_flat_observer_inspector_contract_test.c)
    target_compile_features(jsonl_flat_observer_inspector_contract_test PRIVATE c_std_11)
    target_include_directories(jsonl_flat_observer_inspector_contract_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/cglm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendors/flecs
        ${MDCAD_LIBSOKOL_INCLUDE_DIRS}
        ${MDCAD_LIBCIMGUI_INCLUDE_DIRS}
    )
    target_link_libraries(jsonl_flat_observer_inspector_contract_test PRIVATE libsokol libcimgui flecs_static cjson)
    add_test(NAME jsonl_flat_observer_inspector_contract_test COMMAND jsonl_flat_observer_inspector_contract_test)
endif()

# Emscripten-specific linker options
if (CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    # use our own minimal shell.html
    target_link_options(${PROJECT_NAME} PRIVATE --shell-file ../../web/shell.html)
    # link with WebGPU
    target_link_options(${PROJECT_NAME} PRIVATE "SHELL:--use-port=emdawnwebgpu" -sASYNCIFY)
    # WASM+JS size optimizations
    target_link_options(${PROJECT_NAME} PRIVATE -sNO_FILESYSTEM=1 -sASSERTIONS=0 -sSINGLE_FILE -sMALLOC=emmalloc)
endif()

# explicitly strip dead code (macOS/iOS only - Android uses --gc-sections)
if (CMAKE_C_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_SYSTEM_NAME STREQUAL Emscripten AND NOT CMAKE_SYSTEM_NAME STREQUAL Android)
    target_link_options(${PROJECT_NAME} PRIVATE LINKER:-dead_strip)
endif()

# this hack removes the xxx-CMakeForceLinker.cxx dummy file
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)
endif()

if(MDCAD_NATIVE_DESKTOP)
    set_target_properties(mdcad_math_harness PROPERTIES LINKER_LANGUAGE C)
endif()

#set(CMAKE_CXX_FLAGS "-O3")
