/* Intro scene */

#ifndef _INTRO_H
#define _INTRO_H

#include "scene.h"
#include "resources.h"
#include "label.h"

typedef struct _intro_scene {
    scene _base;
    double time;
    sprite *quaver;
    label *text;
} intro_scene;

intro_scene *intro_scene_create();

#endif
