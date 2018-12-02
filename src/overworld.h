/* Scene for chapter & stage selection */

#ifndef _OVERWORLD_H
#define _OVERWORLD_H

#include "scene.h"
#include "game_data.h"
#include "floue.h"
#include "unveil.h"

#include <SDL.h>

typedef struct _overworld_scene {
    scene _base;
    scene *bg;
    floue *f;
    unveil *u;

    bekter(struct chap_rec *) chaps;
    int n_chaps, cur_chap_idx;

    bekter(SDL_Texture **) stage_tex;
    int cur_stage_idx;

    /* Camera position (centre), in pixels */
    double cam_x, cam_y, cam_scale;
    double cam_targx, cam_targy, cam_targscale;

    /* For switching between chapters */
    int last_chap_idx;
    double since_chap_switch;
} overworld_scene;

overworld_scene *overworld_create(scene *bg);

#endif
