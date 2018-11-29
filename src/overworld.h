/* Scene for chapter & stage selection */

#ifndef _OVERWORLD_H
#define _OVERWORLD_H

#include "scene.h"
#include "game_data.h"
#include <SDL.h>

typedef struct _overworld_scene {
    scene _base;
    scene *bg;

    bekter(struct chap_rec *) chaps;
    int n_chaps, cur_chap_idx;

    bekter(SDL_Texture **) stage_tex;
    int cur_stage_idx;
} overworld_scene;

overworld_scene *overworld_create(scene *bg);

#endif
