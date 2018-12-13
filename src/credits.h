/* The credits page */

#ifndef _CREDITS_H
#define _CREDITS_H

#include "scene.h"
#include "floue.h"
#include "label.h"

typedef struct _credits_scene {
    scene _base;
    scene *bg;

    floue *f;
    label *text;

    double v;
    double p;
} credits_scene;

credits_scene *credits_create(scene *bg);

#endif
