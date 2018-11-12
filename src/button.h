/* Button */

#ifndef _BUTTON_H
#define _BUTTON_H

#include "element.h"

#include <SDL.h>

typedef void (*button_callback)();

typedef struct _button {
    element _base;
    button_callback cb;

    /* For idle, focus, and down */
    SDL_Texture *tex[3];
    SDL_Point sz[3];
    char last_s;

    SDL_Point agl_sz, cur_sz;
    char agl_s;
    unsigned agl_time;
} button;

element *button_create(SDL_Renderer *rdr, button_callback cb,
    const char *img_idle, const char *img_focus, const char *img_down,
    float scale_focus, float scale_down);

#endif
