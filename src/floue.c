#include "floue.h"
#include "global.h"

#include <math.h>
#include <stdlib.h>

static const double SCALE = 4;
static const double V_MAX = WIN_W / 32;
static const double AAA = 0.3;
static const double AA_MAX = 0.5;

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

void floue_add(floue *this, SDL_Point p, SDL_Color c, int sz, double opacity)
{
    if (this->n >= FLOUE_CAP) return;

    this->x[this->n] = p.x;
    this->y[this->n] = p.y;
    this->c[this->n] = c;
    this->sz[this->n] = sz;
    this->v[this->n] = random_in(V_MAX / 4, V_MAX);
    this->a[this->n] = random_abs(M_PI);
    this->aa[this->n] = 0;

    /* Create texture */
    sz /= SCALE;
    SDL_Texture *t = SDL_CreateTexture(g_renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, sz, sz);
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);

    /* Fill with a blurred circle */
    Uint32 *pix = malloc(sz * sz * sizeof(Uint32));
    int i, j;
    for (j = 0; j < sz; ++j)
        for (i = 0; i < sz; ++i) {
            double d = sqrt((double)(sqr(i - sz / 2) + sqr(j - sz / 2))) / (sz / 2);
            double a = (d >= 1 ? 0 :
                d < 0.9 ? 1 : ease_quad_inout((1 - d) * 10)
            );
            pix[j * sz + i] = 0xffffff00 | round(a * opacity * 128);
        }
    SDL_UpdateTexture(t, NULL, pix, sz * sizeof(Uint32));
    free(pix);

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
        this->aa[i] += random_abs(AAA) * dt;
        if (this->aa[i] < -AA_MAX) this->aa[i] = -AA_MAX;
        else if (this->aa[i] > AA_MAX) this->aa[i] = AA_MAX;
        this->a[i] += this->aa[i] * dt;
        /* Update position */
        this->x[i] += this->v[i] * cos(this->a[i]) * dt;
        this->y[i] += this->v[i] * sin(this->a[i]) * dt;
        int sz = this->sz[i];
        if (this->x[i] < -sz / 2) this->x[i] += WIN_W + sz;
        if (this->x[i] > WIN_W + sz / 2) this->x[i] -= WIN_W + sz;
        if (this->y[i] < -sz / 2) this->y[i] += WIN_H + sz;
        if (this->y[i] > WIN_H + sz / 2) this->y[i] -= WIN_H + sz;
    }
}

void floue_draw(floue *this)
{
    if (this->c0.a == 255) {
        SDL_SetRenderDrawColor(g_renderer, this->c0.r, this->c0.g, this->c0.b, 255);
        SDL_RenderClear(g_renderer);
    }
    int i;
    for (i = 0; i < this->n; ++i) {
        SDL_SetTextureColorMod(this->tex[i],
            this->c[i].r, this->c[i].g, this->c[i].b);
        SDL_RenderCopy(g_renderer, this->tex[i], NULL, &(SDL_Rect){
            this->x[i] - this->sz[i] / 2,
            this->y[i] - this->sz[i] / 2,
            this->sz[i], this->sz[i]
        });
    }
}
