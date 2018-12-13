cmake_minimum_required(VERSION 3.8)
project(Gradatim)

add_subdirectory(orion)

if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
    set(SDL2_BUILDING_LIBRARY)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGRADATIM_MINGW")
endif(MINGW)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -fsanitize=address")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -fsanitize=address")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
find_package(SDL2_image REQUIRED)
include_directories(${SDL2_IMAGE_INCLUDE_DIR})
find_package(SDL2_ttf REQUIRED)
include_directories(${SDL2_TTF_INCLUDE_DIR})

add_executable(Gradatim global.c resources.c main.c bekter.c element.c button.c label.c floue.c scene.c dialogue.c transition.c unary_transition.c pause.c sim/schnitt.c sim/sobj.c sim/sim.c game_data.c profile_data.c unveil.c chapfin.c loading.c mod.c particle_sys.c gameplay.c overworld_menu.c overworld.c options.c credits.c couverture.c intro.c)
target_link_libraries(Gradatim orion ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARY} ${SDL2_TTF_LIBRARY})
