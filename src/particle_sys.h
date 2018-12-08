/* A super simple particle system */
/* https://codepen.io/soulwire/pen/foktm */

#ifndef _PARTICLE_SYS_H
#define _PARTICLE_SYS_H

#include <SDL.h>

typedef struct _particle {
    double x, y, vx, vy, drag;
    double angle, wander;
    int w, h;
    double life;
    SDL_Color c;
} particle;

#define PARTICLE_CAP 1024
typedef struct _particle_sys {
    int sz;
    particle pool[PARTICLE_CAP];
} particle_sys;

void particle_init(particle_sys *sys);
void particle_add(particle_sys *sys,
    double x, double y, double vx, double vy, int w, int h,
    double tmin, double tmax,
    SDL_Color c);
void particle_tick(particle_sys *sys, double dt);
void particle_draw_aligned(particle_sys *sys,
    int xoffs, int yoffs, int align);

#endif
