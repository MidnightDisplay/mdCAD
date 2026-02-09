include(FetchContent)

FetchContent_Declare(
    libsokol
    GIT_REPOSITORY https://github.com/floooh/sokol.git
    GIT_TAG master
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(libsokol)

FetchContent_GetProperties(libcimgui)

set(SOKOL_HEADERS
    ${libsokol_SOURCE_DIR}/sokol_gfx.h
    ${libsokol_SOURCE_DIR}/sokol_app.h
    ${libsokol_SOURCE_DIR}/util/sokol_imgui.h
    ${libsokol_SOURCE_DIR}/sokol_glue.h
    ${libsokol_SOURCE_DIR}/sokol_fetch.h
    ${libsokol_SOURCE_DIR}/sokol_log.h
    ${libsokol_SOURCE_DIR}/sokol_time.h)

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    add_library(libsokol STATIC sokol.c ${SOKOL_HEADERS})
    target_compile_options(libsokol PRIVATE -x objective-c)
    set_target_properties(libsokol PROPERTIES LINKER_LANGUAGE OBJC)
    target_link_libraries(libsokol PUBLIC
        "-framework Foundation"
        "-framework UIKit"
        "-framework MetalKit"
        "-framework Metal"
        "-framework AudioToolbox")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_library(libsokol STATIC sokol.c ${SOKOL_HEADERS})
    target_compile_options(libsokol PRIVATE -x objective-c)
    set_target_properties(libsokol PROPERTIES LINKER_LANGUAGE OBJC)
    target_link_libraries(libsokol PUBLIC
        "-framework QuartzCore"
        "-framework Cocoa"
        "-framework MetalKit"
        "-framework Metal")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    add_library(libsokol STATIC sokol.c ${SOKOL_HEADERS})
    target_compile_options(libsokol PRIVATE "SHELL:--use-port=emdawnwebgpu")
    target_link_options(libsokol PUBLIC "SHELL:--use-port=emdawnwebgpu" -sASYNCIFY)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_library(libsokol STATIC sokol.c ${SOKOL_HEADERS})
    target_link_libraries(libsokol PUBLIC
        GLESv3
        EGL
        android
        log)
elseif(WIN32)
    add_library(libsokol STATIC sokol.c ${SOKOL_HEADERS})
    if(MINGW)
        # MinGW: use Vulkan backend
        # Find Vulkan SDK via environment variable
        if(DEFINED ENV{VULKAN_SDK})
            set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
            message(STATUS "Found Vulkan SDK: ${VULKAN_SDK_PATH}")
            target_include_directories(libsokol PUBLIC "${VULKAN_SDK_PATH}/Include")
            target_link_directories(libsokol PUBLIC "${VULKAN_SDK_PATH}/Lib")
            target_link_libraries(libsokol PUBLIC vulkan-1 user32 shell32 gdi32)
        else()
            message(FATAL_ERROR "VULKAN_SDK environment variable not set. Install Vulkan SDK from https://vulkan.lunarg.com/")
        endif()
    else()
        # MSVC: use D3D11
        target_link_libraries(libsokol PUBLIC
            d3d11
            dxgi
            d3dcompiler
            user32
            shell32
            gdi32)
    endif()
else()
    # Linux
    add_library(libsokol STATIC sokol.c ${SOKOL_HEADERS})
    if (CMAKE_SYSTEM_NAME STREQUAL Linux)
        target_link_libraries(libsokol INTERFACE X11 Xi Xcursor GL dl m)
        target_link_libraries(libsokol PUBLIC Threads::Threads)
    endif()
endif()

target_include_directories(libsokol PRIVATE
    ${libsokol_SOURCE_DIR}
    ${libsokol_SOURCE_DIR}/util
    ${libcimgui_SOURCE_DIR}  # Path to cimgui.h
)

target_link_libraries(libsokol PUBLIC libcimgui)

target_include_directories(
    libsokol
    INTERFACE
    ${libsokol_SOURCE_DIR}
    ${libsokol_SOURCE_DIR}/util
)