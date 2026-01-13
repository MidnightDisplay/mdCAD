set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(SOURCES
    demo.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} libsokol)

# Emscripten-specific linker options
if (CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    # use our own minimal shell.html
    target_link_options(${PROJECT_NAME} PRIVATE --shell-file ../../web/shell.html)
    # link with WebGL2
    target_link_options(${PROJECT_NAME} PRIVATE -sUSE_WEBGL2=1)
    # WASM+JS size optimizations
    target_link_options(${PROJECT_NAME} PRIVATE -sNO_FILESYSTEM=1 -sASSERTIONS=0 -sSINGLE_FILE -sMALLOC=emmalloc --closure=1)
endif()

# explicitly strip dead code
if (CMAKE_C_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    target_link_options(${PROJECT_NAME} PRIVATE LINKER:-dead_strip)
endif()

# this hack removes the xxx-CMakeForceLinker.cxx dummy file
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)