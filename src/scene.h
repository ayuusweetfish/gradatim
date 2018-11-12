/* Base struct definition for scenes, plus a sample implementation */

#ifndef _SCENE_H
#define _SCENE_H

#include "bekter.h"

#include <SDL.h>

struct _scene;
typedef void (*scene_tick_func)(struct _scene *this, double dt);
typedef void (*scene_draw_func)(struct _scene *this);
typedef void (*scene_drop_func)(struct _scene *this);

typedef struct _scene {
    SDL_Renderer *renderer;
    bekter(element *) children;

    scene_tick_func tick;
    scene_draw_func draw;
    scene_drop_func drop;
} scene;

#define scene_tick(__sp, __dt) do { \
    scene_tick_children((scene *)(__sp), __dt); \
    if (((scene *)(__sp))->tick) \
        (((scene *)(__sp))->tick((scene *)(__sp), __dt)); \
} while (0)
#define scene_draw(__sp)        if ((__sp)->draw) ((__sp)->draw(__sp))
#define scene_drop(__sp) do { \
    scene_clear_children((scene *)(__sp)); \
    if (((scene *)(__sp))->drop) ((scene *)(__sp))->drop((scene *)__sp); \
    free(__sp); \
} while (0)

void scene_handle_mousemove(scene *this, SDL_MouseMotionEvent *ev);
void scene_handle_mousebutton(scene *this, SDL_MouseButtonEvent *ev);
void scene_tick_children(scene *this, double dt);
void scene_clear_children(scene *this);

typedef struct _colour_scene {
    scene _base;
    unsigned char r, g, b;
} colour_scene;

scene *colour_scene_create(SDL_Renderer *rdr, int r, int g, int b);

#endif
