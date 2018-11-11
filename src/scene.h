/* Base struct definition for scenes, plus a sample implementation */

#ifndef _SCENE_H
#define _SCENE_H

#include "bekter.h"

#include <SDL.h>

struct _scene;
typedef void (*scene_tick_func)(struct _scene *this, double dt);
typedef void (*scene_draw_func)(struct _scene *this);

typedef struct _scene {
    SDL_Renderer *renderer;

    scene_tick_func tick;
    scene_draw_func draw;
} scene;

typedef struct _colour_scene {
    scene _base;
    unsigned char r, g, b;
} colour_scene;

scene *colour_scene_create(SDL_Renderer *rdr, int r, int g, int b);
void colour_scene_tick(colour_scene *this, double dt);
void colour_scene_draw(colour_scene *this);

#endif
