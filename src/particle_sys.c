#include "particle_sys.h"
#include "global.h"

#include <string.h>

void particle_init(particle_sys *sys)
{
    sys->sz = 0;
}

void particle_add(particle_sys *sys,
    double x, double y, SDL_Color c)
{
    /* TODO: Eliminate earliest particles in the pool */
    if (sys->sz >= PARTICLE_CAP) return;
    particle p;
    p.x = x;
    p.y = y;
    p.vx = 100;
    p.vy = 100;
    p.drag = 0.92;
    p.angle = 0;
    p.wander = 0;
    p.w = p.h = 16;
    p.c = c;
    sys->pool[sys->sz++] = p;
}

void particle_tick(particle_sys *sys, double dt)
{
    int i, n = sys->sz;
    for (i = 0; i < n; ++i) {
        particle *p = &sys->pool[i];
        p->x += p->vx * dt;
        p->y += p->vy * dt;
    }
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
        SDL_SetRenderDrawColor(g_renderer, p.c.r, p.c.g, p.c.b, 255);
        SDL_RenderFillRect(g_renderer, &r);
    }
}
