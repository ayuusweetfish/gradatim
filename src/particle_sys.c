#include "particle_sys.h"
#include "global.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

void particle_init(particle_sys *sys)
{
    sys->sz = 0;
}

static inline double rand_in(double a, double b)
{
    return (double)rand() / RAND_MAX * (b - a) + a;
}

void particle_add(particle_sys *sys,
    double x, double y, double vx, double vy, int w, int h,
    double tmin, double tmax,
    unsigned char r, unsigned char g, unsigned char b)
{
    /* TODO: Eliminate earliest particles in the pool */
    if (sys->sz >= PARTICLE_CAP) return;
    particle p;
    p.x = x;
    p.y = y;
    p.vx = vx;
    p.vy = vy;
    p.drag = 3;
    p.angle = rand_in(-M_PI, +M_PI);
    p.wander = 15;
    p.w = w;
    p.h = h;
    p.life = rand_in(tmin, tmax);
    p.r = r;
    p.g = g;
    p.b = b;
    sys->pool[sys->sz++] = p;
}

void particle_tick(particle_sys *sys, double dt)
{
    int i, n = sys->sz;
    for (i = 0; i < n; ++i) {
        particle *p = &sys->pool[i];
        if ((p->life -= dt) <= 0) {
            *p = sys->pool[--n];
            --i;
            continue;
        }
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        p->vx *= (1 - p->drag * dt);
        p->vy *= (1 - p->drag * dt);
        p->angle += rand_in(-1, +1) * p->wander * dt;
        p->vx += cos(p->angle) * 10 * dt;
        p->vy += sin(p->angle) * 10 * dt;
    }
    sys->sz = n;
}

void particle_draw_aligned(particle_sys *sys,
    int xoffs, int yoffs, int align)
{
    int i, n = sys->sz;
    for (i = 0; i < n; ++i) {
        particle p = sys->pool[i];
        SDL_Rect r;
        r.x = iround((p.x + xoffs) / align) * align;
        r.y = iround((p.y + yoffs) / align) * align;
        r.w = p.w;
        r.h = p.h;
        SDL_SetRenderDrawColor(g_renderer, p.r, p.g, p.b, 255);
        SDL_RenderFillRect(g_renderer, &r);
    }
}
