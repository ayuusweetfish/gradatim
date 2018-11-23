/* Pause scene */

#ifndef _PAUSE_H
#define _PAUSE_H

#include "scene.h"

typedef struct _pause_scene {
    scene _base;
    scene *a, *b, **p;

    SDL_Texture *a_tex;
} pause_scene;

/* `a` is the scene to Resume to, and `b` is the scene to Go Back to
 * When Go Back is selected, `a` will be dropped as well */
pause_scene *pause_scene_create(scene **a, scene *b);

#endif
