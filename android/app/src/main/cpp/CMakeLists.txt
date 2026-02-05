# CMake wrapper for Android builds
# This file is referenced by Gradle and includes the main project

cmake_minimum_required(VERSION 3.22)
project(mdCAD_android)

# Path to the main project root (5 levels up from android/app/src/main/cpp/)
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../")

# Add the main project
add_subdirectory(${PROJECT_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/mdCAD)
