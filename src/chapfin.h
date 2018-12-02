/* Scene for the end of a chapter */

#ifndef _CHAPFIN_H
#define _CHAPFIN_H

#include "scene.h"
#include "label.h"
#include "gameplay.h"
#include "unveil.h"
#include "floue.h"

typedef struct _chapfin_scene {
    scene _base;

    gameplay_scene *g;

    double time;
    double orig_cam_y;

    unveil *u;
    floue *f;
    label *l_num, *l_title;
    label *l_extra;
} chapfin_scene;

chapfin_scene *chapfin_scene_create(gameplay_scene *g);

#endif
