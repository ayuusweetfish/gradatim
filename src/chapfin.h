/* Scene for the end of a chapter */

#ifndef _CHAPFIN_H
#define _CHAPFIN_H

#include "scene.h"
#include "element.h"
#include "label.h"
#include "gameplay.h"
#include "mod.h"
#include "unveil.h"
#include "floue.h"

#include <stdbool.h>

typedef struct _chapfin_scene {
    scene _base;

    gameplay_scene *g;

    double time;
    double orig_cam_y;

    unveil *u;
    floue *f;
    label *l_num, *l_title;

    bool summary_shown;
    sprite *mod_icon[N_MODS];
    label *l_summary;
    sprite *clock, *mushroom;
    label *l_timer, *l_timer_d, *l_retries;
} chapfin_scene;

chapfin_scene *chapfin_scene_create(gameplay_scene *g);

#endif
