/* The scene for gameplay */

#ifndef _GAMEPLAY_H
#define _GAMEPLAY_H

#include "scene.h"
#include "sim/sim.h"

typedef struct _gameplay_scene {
    scene _base;
    scene *bg, **bg_ptr;

    sim *simulator;
} gameplay_scene;

gameplay_scene *gameplay_scene_create(scene **bg);

#endif
