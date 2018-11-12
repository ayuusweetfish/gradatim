/* Scenes that take two scenes and manipulate on them */

#ifndef _TRANSITION_H
#define _TRANSITION_H

#include "scene.h"

#include <stdbool.h>

struct _transition_scene;
typedef void (*transition_draw_func)(struct _transition_scene *this);

typedef struct _transition_scene {
    scene _base;
    scene *a, *b, **p;
    bool preserves_a;
    double duration, elapsed;

    SDL_Texture *a_tex, *b_tex;

    transition_draw_func t_draw;
} transition_scene;

void transition_set_preservative(transition_scene *this);
scene *transition_slidedown_create(scene **a, scene *b, double dur);
scene *transition_slideup_create(scene **a, scene *b, double dur);

#endif
