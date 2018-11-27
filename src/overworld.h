/* Scene for chapter & stage selection */

#ifndef _OVERWORLD_H
#define _OVERWORLD_H

#include "scene.h"
#include "game_data.h"

typedef struct _overworld_scene {
    scene _base;
    scene *bg;

    bekter(struct chap_rec) chaps;
    int n_chaps;
} overworld_scene;

overworld_scene *overworld_create(scene *bg);

#endif
