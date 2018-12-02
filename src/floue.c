#include "floue.h"
#include "global.h"

#include <stdlib.h>

static const double SCALE = 4;
static const int SZ_W = WIN_W / SCALE;
static const int SZ_H = WIN_H / SCALE;

static const double TR_DUR = 2;
static const double MAX_MOV = SZ_H / 25;
static const double MAX_ALT = 0.1;

static inline double random_in(double l, double h)
{
    return (double)rand() / RAND_MAX * (h - l) + l;
}

static inline void update_tex(floue *this)
{
    /* Swap textures */
    SDL_Texture *t = this->a;
    this->a = this->b;
    this->b = t;

    /* Alter the points */
    int i;
    for (i = 0; i < this->n; ++i) {
        double dx_min = -this->p[i].x, dx_max = SZ_W - this->p[i].x;
        double dy_min = -this->p[i].y, dy_max = SZ_H - this->p[i].y;
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
                double w = exp(-d / 2500.0);
                r += this->c[k].r * w;
                g += this->c[k].g * w;
                b += this->c[k].b * w;
                v += w;
            }
            //printf("%d | %f %f %f %f\n", this->n, r, g, b, v);
            this->pix[j * SZ_W + i] =
                ((int)(r / v) << 24) | ((int)(g / v) << 16) | ((int)(b / v) << 8) | 0xff;
        }
    SDL_UpdateTexture(this->b, NULL, this->pix, SZ_W * 4);
}

floue *floue_create(SDL_Color c0, double v0)
{
    floue *this = malloc(sizeof(floue));
    this->a = SDL_CreateTexture(g_renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC,
        SZ_W, SZ_H);
    this->b = SDL_CreateTexture(g_renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC,
        SZ_W, SZ_H);
    this->pix = malloc(SZ_W * SZ_H * sizeof(Uint32));
    this->c0 = c0;
    this->v0 = v0;
    this->n = 0;
    this->time = 0;
    update_tex(this);
    update_tex(this);
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
    SDL_DestroyTexture(this->a);
    SDL_DestroyTexture(this->b);
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
    SDL_RenderCopy(g_renderer, this->b, NULL, NULL);
}
