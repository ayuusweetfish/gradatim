#include "floue.h"
#include "global.h"

#include <math.h>
#include <stdlib.h>

static const double SCALE = 4;
static const double ACCEL = WIN_W / 20;
static const double WANDER = 1;
static const double V_MAX = WIN_W / 4;

static inline double random_abs(double max_abs)
{
    return (double)rand() / RAND_MAX * (2 * max_abs) - max_abs;
}

static inline double random_in(double l, double h)
{
    return (double)rand() / RAND_MAX * (h - l) + l;
}

floue *floue_create(SDL_Color c0)
{
    floue *this = malloc(sizeof(floue));
    this->c0 = c0;
    this->n = 0;
    return this;
}

void floue_add(floue *this, SDL_Point p, SDL_Color c, int sz)
{
    if (this->n >= FLOUE_CAP) return;

    this->x[this->n] = p.x;
    this->y[this->n] = p.y;
    this->sz[this->n] = sz;
    this->vx[this->n] = random_abs(ACCEL / 4);
    this->vy[this->n] = random_abs(ACCEL / 4);

    /* Create texture */
    sz /= SCALE;
    SDL_Texture *t = SDL_CreateTexture(g_renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, sz, sz);
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);

    /* Fill with a blurred circle */
    Uint32 *pix = malloc(sz * sz * sizeof(Uint32));
    Uint32 base = (c.r << 24) | (c.g << 16) | (c.b << 8);
    int i, j;
    for (j = 0; j < sz; ++j)
        for (i = 0; i < sz; ++i) {
            double d = sqrt((double)(sqr(i - sz / 2) + sqr(j - sz / 2))) / (sz / 2);
            double opacity = (d >= 1 ? 0 :
                (d < 0.5 ? 1 - d * d * d * 4 : (1 - d) * (1 - d) * (1 - d) * 4)
            );
            pix[j * sz + i] = base | round(opacity * 128);
        }
    SDL_UpdateTexture(t, NULL, pix, sz * sizeof(Uint32));

    this->tex[this->n++] = t;
}

void floue_drop(floue *this)
{
    int i;
    for (i = 0; i < this->n; ++i)
        SDL_DestroyTexture(this->tex[i]);
    free(this);
}

void floue_tick(floue *this, double dt)
{
    int i;
    for (i = 0; i < this->n; ++i) {
        /* Update velocity */
        double vx = this->vx[i], vy = this->vy[i];
        double a = atan2(vy, vx) + random_abs(WANDER * dt);
        double v = sqrt(sqr(vx) + sqr(vy));
        double v_max = v + ACCEL * dt, v_min = v - ACCEL * dt;
        if (v_max > V_MAX) v_max = V_MAX;
        if (v_min < 0) v_min = 0;
        v = random_in(v_min, v_max);
        if (isnan(a)) a = random_abs(M_PI);
        vx = v * cos(a);
        vy = v * sin(a);
        /* Update position */
        this->vx[i] = vx;
        this->vy[i] = vy;
        this->x[i] += vx * dt;
        this->y[i] += vy * dt;
        int sz = this->sz[i];
        if (this->x[i] < -sz / 2) this->x[i] += WIN_W + sz;
        if (this->x[i] > WIN_W + sz / 2) this->x[i] -= WIN_W + sz;
        if (this->y[i] < -sz / 2) this->y[i] += WIN_H + sz;
        if (this->y[i] > WIN_H + sz / 2) this->y[i] -= WIN_H + sz;
    }
}

void floue_draw(floue *this)
{
    SDL_SetRenderDrawColor(g_renderer, this->c0.r, this->c0.g, this->c0.b, 255);
    SDL_RenderClear(g_renderer);
    int i;
    for (i = 0; i < this->n; ++i)
        SDL_RenderCopy(g_renderer, this->tex[i], NULL, &(SDL_Rect){
            this->x[i] - this->sz[i] / 2,
            this->y[i] - this->sz[i] / 2,
            this->sz[i], this->sz[i]
        });
}