/* Basic reusable elements that can be placed in a scene */

#ifndef _ELEMENT_H
#define _ELEMENT_H

#include <SDL.h>

struct _element;
typedef void (*element_tick_func)(struct _element *this, double dt);
typedef void (*element_draw_func)(struct _element *this);
typedef void (*element_drop_func)(struct _element *this);

typedef struct _element {
    SDL_Renderer *renderer;
    SDL_Rect dim;

    element_tick_func tick;
    element_draw_func draw;
    element_drop_func drop;
} element;

#define element_tick(__sp, __dt)    ((__sp)->tick(__sp, __dt))
#define element_draw(__sp)          ((__sp)->draw(__sp))
#define element_drop(__sp)          ((__sp)->drop(__sp))

void element_place(element *e, int x, int y);
void element_place_anchored(element *e, float x, float y, float ax, float ay);

typedef struct _sprite {
    element _base;
    SDL_Texture *tex;
} sprite;

element *sprite_create(SDL_Renderer *rdr, const char *path);

#endif
