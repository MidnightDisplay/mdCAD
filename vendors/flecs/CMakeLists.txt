# Flecs ECS Library
# Single-header amalgamation version

add_library(flecs_static STATIC flecs.c)

target_include_directories(flecs_static
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Flecs compile definitions
# Disable HTTP/REST features that require sockets (not available in Emscripten)
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_compile_definitions(flecs_static
        PUBLIC
        FLECS_NO_HTTP
        FLECS_NO_REST
        FLECS_NO_APP
    )
endif()

# On some platforms, we may need threading support
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(flecs_static PUBLIC Threads::Threads)
endif()

# Android requires log library for debugging
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(flecs_static PUBLIC log)
endif()
