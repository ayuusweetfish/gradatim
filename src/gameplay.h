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
    enum { HOR_STATE_NONE, HOR_STATE_LEFT, HOR_STATE_RIGHT } hor_state, facing;
    enum { VER_STATE_NONE, VER_STATE_UP, VER_STATE_DOWN } ver_state;
    /* Special movement states */
    enum movement_state {
        MOV_NORMAL = 0, MOV_ANTHOP = 1,
        /* Dash directions can be distinguished by taking & 3 */
        MOV_DASH_BASE = 4,
        MOV_HORDASH = MOV_DASH_BASE + 1,
        MOV_VERDASH = MOV_DASH_BASE + 2,
        MOV_DIAGDASH = MOV_DASH_BASE + 3,
        /* Bitmasks denoting dash direction */
        MOV_DASH_LEFT = 8,
        MOV_DASH_UP = 16
    } mov_state;
    double mov_time;    /* Time remaining until movement state resets */
} gameplay_scene;

gameplay_scene *gameplay_scene_create(scene **bg);

#endif
