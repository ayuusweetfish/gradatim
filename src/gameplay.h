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
    double cam_x, cam_y;

    /* Input state */
    enum { HOR_STATE_NONE, HOR_STATE_LEFT, HOR_STATE_RIGHT } hor_state;
    enum { VER_STATE_NONE, VER_STATE_UP, VER_STATE_DOWN } ver_state;
    double last_hop_press;  /* Time of last hop button press, in beats */
} gameplay_scene;

gameplay_scene *gameplay_scene_create(scene **bg);

#endif
