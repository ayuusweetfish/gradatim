/* The scene for gameplay */

#ifndef _GAMEPLAY_H
#define _GAMEPLAY_H

#include "resources.h"
#include "scene.h"
#include "sim/sim.h"

typedef struct _gameplay_scene {
    scene _base;
    scene *bg, **bg_ptr;

    sim *simulator;
    double rem_time;

    texture prot_tex;
    texture grid_tex[256];
    /* The position of the camera's top-left corner in the
     * simulated world, expressed in units */
    float cam_x, cam_y;
} gameplay_scene;

gameplay_scene *gameplay_scene_create(scene **bg);

#endif
