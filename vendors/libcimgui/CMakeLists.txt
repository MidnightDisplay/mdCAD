include(FetchContent)

FetchContent_Declare(
    libcimgui
    GIT_REPOSITORY https://github.com/cimgui/cimgui.git
    GIT_TAG 1.92.5dock
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(libcimgui)

add_library(libcimgui STATIC
    ${libcimgui_SOURCE_DIR}/cimgui.cpp
    ${libcimgui_SOURCE_DIR}/cimgui.h
    ${libcimgui_SOURCE_DIR}/imgui/imgui.cpp
    ${libcimgui_SOURCE_DIR}/imgui/imgui.h
    ${libcimgui_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${libcimgui_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${libcimgui_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${libcimgui_SOURCE_DIR}/imgui/imgui_demo.cpp)

target_include_directories(
    libcimgui
    INTERFACE
    ${libcimgui_SOURCE_DIR}
)