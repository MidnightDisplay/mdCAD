cmake_minimum_required(VERSION 3.0...3.25)

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

set(FETCHCONTENT_QUIET OFF)

project(
    skl_tmp
    VERSION 0.1.0
    LANGUAGES CXX C OBJC
    )

# set(DAWN_FETCH_DEPENDENCIES ON)
# add_subdirectory("dawn" EXCLUDE_FROM_ALL)

add_subdirectory(vendors)
add_subdirectory(src)