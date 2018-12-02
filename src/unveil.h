/* A white texture that unveils gradually */

#ifndef _UNVEIL_H
#define _UNVEIL_H

#include <SDL.h>

typedef struct _unveil {
    SDL_Texture *tex;
    float *val;
} unveil;

unveil *unveil_create();
void unveil_drop(unveil *this);
void unveil_draw(unveil *this, double time, double opacity);

#endif
