/* Scenes that take two scenes and manipulate on them */

#ifndef _TRANSITION_H
#define _TRANSITION_H

#include "scene.h"

typedef struct _transition_scene {
    scene _base;
    scene *a, *b, **p;
    double duration, elapsed;

    SDL_Renderer *a_rdr, *b_rdr;
    SDL_Texture *a_tex, *b_tex;
} transition_scene;

void transition_tick(transition_scene *this, double dt);

scene *transition_slidedown_create(
    SDL_Window *win, SDL_Renderer *rdr, scene **a, scene *b, double dur);
void transition_slidedown_draw(transition_scene *this);

#endif
