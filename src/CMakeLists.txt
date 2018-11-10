cmake_minimum_required(VERSION 3.8)
project(Gradatim)

add_subdirectory(orion)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

add_executable(g main.c)
target_link_libraries(g orion ${SDL2_LIBRARY})
