cmake_minimum_required(VERSION 3.0...3.25)

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

set(FETCHCONTENT_QUIET OFF)

# Android doesn't support Objective-C
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    # 16KB page alignment for Android 15+ compatibility
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,max-page-size=16384")
    project(
        skl_tmp
        VERSION 0.1.0
        LANGUAGES CXX C
    )
else()
    project(
        skl_tmp
        VERSION 0.1.0
        LANGUAGES CXX C OBJC
    )
endif()

# set(DAWN_FETCH_DEPENDENCIES ON)
# add_subdirectory("dawn" EXCLUDE_FROM_ALL)

add_subdirectory(vendors)
add_subdirectory(src)