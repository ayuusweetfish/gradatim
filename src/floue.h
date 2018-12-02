/* A blurred texture */

#ifndef _FLOUE_H
#define _FLOUE_H

#include <SDL.h>

#define FLOUE_CAP 16

typedef struct _floue {
    SDL_Color c0;
    int n;
    SDL_Texture *tex[FLOUE_CAP];
    SDL_Point p[FLOUE_CAP];
    int sz[FLOUE_CAP];
} floue;

floue *floue_create(SDL_Color c0);
void floue_add(floue *this, SDL_Point p, SDL_Color c, int sz);
void floue_drop(floue *this);
void floue_tick(floue *this, double dt);
void floue_draw(floue *this);

#endif
