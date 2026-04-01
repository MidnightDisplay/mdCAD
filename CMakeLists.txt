cmake_minimum_required(VERSION 3.0...3.25)

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

set(FETCHCONTENT_QUIET OFF)

# Only Apple platforms need Objective-C
if(APPLE)
    project(
        mdCAD
        VERSION 0.1.0
        LANGUAGES CXX C OBJC
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    # 16KB page alignment for Android 15+ compatibility
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,max-page-size=16384")
    project(
        mdCAD
        VERSION 0.1.0
        LANGUAGES CXX C
    )
else()
    # Windows, Linux, etc.
    project(
        mdCAD
        VERSION 0.1.0
        LANGUAGES CXX C
    )
endif()

include(CTest)

#------------------------------------------------------------------------------
# Build Configuration (Release/Debug optimization levels)
#------------------------------------------------------------------------------

# Set available build types for multi-config generators (Visual Studio, Xcode)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel" CACHE STRING "" FORCE)

# Default to Release if not specified (for single-config generators like Ninja, Make)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

# Compiler-specific optimization and debug flags
if(MSVC)
    # Visual Studio / MSVC compiler
    set(CMAKE_C_FLAGS_DEBUG "/Od /Zi /RTC1" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /RTC1" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_RELEASE "/O2 /GL /DNDEBUG" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /GL /DNDEBUG" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "/O2 /Zi /DNDEBUG" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /Zi /DNDEBUG" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_MINSIZEREL "/O1 /DNDEBUG" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_MINSIZEREL "/O1 /DNDEBUG" CACHE STRING "" FORCE)
    # Link-time optimization for Release
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG" CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "/LTCG" CACHE STRING "" FORCE)
else()
    # GCC, Clang, MinGW, and other compilers
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG" CACHE STRING "" FORCE)
    
    # Enable link-time optimization for Release (GCC/Clang)
    if(NOT EMSCRIPTEN AND NOT ANDROID)
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto" CACHE STRING "" FORCE)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto" CACHE STRING "" FORCE)
    endif()
endif()

# Display active build configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C compiler: ${CMAKE_C_COMPILER_ID}")
message(STATUS "CXX compiler: ${CMAKE_CXX_COMPILER_ID}")

# set(DAWN_FETCH_DEPENDENCIES ON)
# add_subdirectory("dawn" EXCLUDE_FROM_ALL)

add_subdirectory(vendors)
add_subdirectory(src)
