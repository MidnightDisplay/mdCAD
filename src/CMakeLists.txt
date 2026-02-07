set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(SOURCES
    app.c
)

# Add platform-specific pick buffer readback implementations
if(APPLE)
    list(APPEND SOURCES gpu/pick_readback_metal.m)
elseif(WIN32 AND NOT MINGW)
    # D3D11 readback for MSVC builds
    list(APPEND SOURCES gpu/pick_readback_d3d11.c)
elseif(MINGW)
    # OpenGL readback for MinGW builds
    list(APPEND SOURCES gpu/pick_readback_opengl.c)
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

target_link_libraries(${PROJECT_NAME} libsokol libcimgui flecs_static)

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

#set(CMAKE_CXX_FLAGS "-O3")