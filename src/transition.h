/* Scenes that take two scenes and manipulate on them */

#ifndef _TRANSITION_H
#define _TRANSITION_H

#include "scene.h"

typedef struct _transition_scene {
    scene _base;
    scene *a, *b, **p;
    double duration, elapsed;

    SDL_Texture *a_tex, *b_tex;
    SDL_Texture *orig_target;   /* Original render target */
} transition_scene;

void transition_tick(transition_scene *this, double dt);

scene *transition_slidedown_create(scene **a, scene *b, double dur);
void transition_slidedown_draw(transition_scene *this);

#endif
