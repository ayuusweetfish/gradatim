#include "floue.h"
#include "global.h"

#include <stdlib.h>

static const double SCALE = 6;
static const int SZ_W = WIN_W / SCALE;
static const int SZ_H = WIN_H / SCALE;
static const int MARGIN = SZ_W / 3;

static const double RANGE_FACTOR = 20.0;

static const double TR_DUR = 1;
static const double MAX_MOV = SZ_H / 8;
static const double MAX_ALT = 10;

static inline double random_in(double l, double h)
{
    return (double)rand() / RAND_MAX * (h - l) + l;
}

static inline void update_tex(floue *this)
{
    /* Swap textures */
    SDL_Texture *t = this->tex[0];
    int i;
    for (i = 0; i < FLOUE_LAYERS - 1; ++i)
        this->tex[i] = this->tex[i + 1];
    this->tex[FLOUE_LAYERS - 1] = t;

    /* Alter the points */
    for (i = 0; i < this->n; ++i) {
        double dx_min = -MARGIN - this->p[i].x, dx_max = SZ_W + MARGIN - this->p[i].x;
        double dy_min = -MARGIN - this->p[i].y, dy_max = SZ_H + MARGIN - this->p[i].y;
        if (dx_min < -MAX_MOV) dx_min = -MAX_MOV;
        if (dx_max > +MAX_MOV) dx_max = +MAX_MOV;
        if (dy_min < -MAX_MOV) dy_min = -MAX_MOV;
        if (dy_max > +MAX_MOV) dy_max = +MAX_MOV;
        this->p[i].x += random_in(dx_min, dx_max);
        this->p[i].y += random_in(dy_min, dy_max);
        this->v[i] += random_in(-MAX_ALT, +MAX_ALT);
    }

    /* Build pixel data */
    int j, k;
    for (j = 0; j < SZ_H; ++j)
        for (i = 0; i < SZ_W; ++i) {
            double r = this->c0.r, g = this->c0.g, b = this->c0.b;
            double v = this->v0;
            for (k = 0; k < this->n; ++k) {
                double d = sqr(this->p[k].x - i) + sqr(this->p[k].y - j);
                double w = exp(-d / RANGE_FACTOR) * this->v[k];
                r += this->c[k].r * w;
                g += this->c[k].g * w;
                b += this->c[k].b * w;
                v += w;
            }
            //printf("%d | %f %f %f %f\n", this->n, r, g, b, v);
            this->pix[j * SZ_W + i] =
                ((int)(r / v) << 24) | ((int)(g / v) << 16) | ((int)(b / v) << 8) | 0xff;
        }
    SDL_UpdateTexture(this->tex[FLOUE_LAYERS - 1], NULL, this->pix, SZ_W * 4);
}

floue *floue_create(SDL_Color c0, double v0)
{
    floue *this = malloc(sizeof(floue));
    int i;
    for (i = 0; i < FLOUE_LAYERS; ++i) {
        this->tex[i] = SDL_CreateTexture(g_renderer,
            SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC,
            SZ_W, SZ_H);
        SDL_SetTextureBlendMode(this->tex[i], SDL_BLENDMODE_BLEND);
    }
    this->pix = malloc(SZ_W * SZ_H * sizeof(Uint32));
    this->c0 = c0;
    this->v0 = v0;
    this->n = 0;
    this->time = 0;
    for (i = 0; i < FLOUE_LAYERS; ++i) update_tex(this);
    return this;
}

void floue_add(floue *this, SDL_Point p, SDL_Color c, double v)
{
    if (this->n >= FLOUE_CAP) return;
    this->p[this->n] = (SDL_Point){p.x / SCALE, p.y / SCALE};
    this->c[this->n] = c;
    this->v[this->n++] = v;
}

void floue_drop(floue *this)
{
    int i;
    for (i = 0; i < FLOUE_LAYERS; ++i)
        SDL_DestroyTexture(this->tex[i]);
    free(this->pix);
    free(this);
}

void floue_tick(floue *this, double dt)
{
    if ((this->time += dt) >= TR_DUR) {
        update_tex(this);
        this->time -= TR_DUR;
    }
}

void floue_draw(floue *this)
{
    int i;
    for (i = 0; i < FLOUE_LAYERS; ++i) {
        double opacity = (i == 0 ? 1 : (1 - (i - this->time / TR_DUR) / (FLOUE_LAYERS - 1)));
        opacity *= opacity;
        opacity *= opacity;
        SDL_SetTextureAlphaMod(this->tex[i], round(opacity * 255));
        SDL_RenderCopy(g_renderer, this->tex[i], NULL, NULL);
    }
}
