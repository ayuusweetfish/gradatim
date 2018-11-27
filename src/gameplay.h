/* The scene for gameplay */

#ifndef _GAMEPLAY_H
#define _GAMEPLAY_H

#include "resources.h"
#include "scene.h"
#include "sim/sim.h"
#include "stage_rec.h"

typedef struct _gameplay_scene {
    scene _base;
    scene *bg, **bg_ptr;

    /* Struct holding all chapter-specific data */
    struct chap_rec *chap;
    int cur_stage_idx;
    /* Struct holding all stage-specific data
     * (objects, dialogues, textures etc.) */
    struct stage_rec *rec;
    sim *simulator, *prev_sim;
    double rem_time;
    double aud_sim_offset;
    int aud_sim_offset_n_samples;
    unsigned int dialogue_triggered;

    /* Display states */
    enum display_state {
        DISP_NORMAL,    /* Normal simulation and gameplay */
        DISP_LEADIN,    /* Flashlight lead in */
        DISP_FAILURE    /* Failure animation */
    } disp_state;
    double disp_time;   /* Remaining time of global animation, in seconds */
    SDL_Texture *leadin_tex;

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
