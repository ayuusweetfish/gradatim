/* Pause scene */

#ifndef _PAUSE_H
#define _PAUSE_H

#include "scene.h"

typedef struct _pause_scene {
    scene _base;
    scene *a, *b, **p;

    SDL_Texture *a_tex;
} pause_scene;

pause_scene *pause_scene_create(scene **a, scene *b);

#endif
