#include "floue.h"
#include "global.h"

#include <stdlib.h>

static const double SCALE = 4;
static const int SZ_W = WIN_W / SCALE;
static const int SZ_H = WIN_H / SCALE;
static const int MARGIN = SZ_W / 3;

static const double TR_DUR = 1;
static const double MAX_MOV = SZ_H / 8;
static const double MAX_ALT = 10;

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

    this->p[this->n] = p;
    this->sz[this->n] = sz;

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
}

void floue_draw(floue *this)
{
    SDL_SetRenderDrawColor(g_renderer, this->c0.r, this->c0.g, this->c0.b, 255);
    SDL_RenderClear(g_renderer);
    int i;
    for (i = 0; i < this->n; ++i)
        SDL_RenderCopy(g_renderer, this->tex[i], NULL, &(SDL_Rect){
            this->p[i].x - this->sz[i] / 2,
            this->p[i].y - this->sz[i] / 2,
            this->sz[i], this->sz[i]
        });
}
