/* A blurred texture */

#ifndef _FLOUE_H
#define _FLOUE_H

#include <SDL.h>

#define FLOUE_CAP 16
#define FLOUE_LAYERS 8

typedef struct _floue {
    SDL_Texture *tex[FLOUE_LAYERS];

    SDL_Color c0;
    double v0;

    int n;
    SDL_Point p[FLOUE_CAP];
    SDL_Color c[FLOUE_CAP];
    double v[FLOUE_CAP];

    Uint32 *pix;
    double time;
} floue;

floue *floue_create(SDL_Color c0, double v0);
void floue_add(floue *this, SDL_Point p, SDL_Color c, double v);
void floue_drop(floue *this);
void floue_tick(floue *this, double dt);
void floue_draw(floue *this);

#endif
