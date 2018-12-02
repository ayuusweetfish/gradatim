cmake_minimum_required(VERSION 3.8)
project(Gradatim)

add_subdirectory(orion)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
find_package(SDL2_image REQUIRED)
include_directories(${SDL2_IMAGE_INCLUDE_DIR})
find_package(SDL2_ttf REQUIRED)
include_directories(${SDL2_TTF_INCLUDE_DIR})

add_executable(g global.c resources.c main.c bekter.c element.c button.c label.c floue.c scene.c dialogue.c transition.c unary_transition.c pause.c sim/schnitt.c sim/sobj.c sim/sim.c game_data.c unveil.c loading.c mod.c gameplay.c overworld_menu.c overworld.c)
target_link_libraries(g orion ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARY} ${SDL2_TTF_LIBRARY})
