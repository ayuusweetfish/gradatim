#include "unveil.h"
#include "global.h"

#include <stdlib.h>

static const double SCALE = 4;
static const int W = WIN_W / SCALE;
static const int H = WIN_H / SCALE;
static const float NEIGHBOUR = 0.1;

unveil *unveil_create()
{
    unveil *this = malloc(sizeof(unveil));
    this->tex = SDL_CreateTexture(g_renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, W, H);
    SDL_SetTextureBlendMode(this->tex, SDL_BLENDMODE_BLEND);

    /* TODO: Generate a continuous matrix */
    this->val = malloc(W * H * sizeof(float));
    int i, j;
    for (i = 0; i < H; ++i)
        for (j = 0; j < W; ++j)
            this->val[i * W + j] = (float)i / H / 2 + (float)j / W / 2;

    return this;
}

void unveil_drop(unveil *this)
{
    SDL_DestroyTexture(this->tex);
    free(this);
}

void unveil_draw(unveil *this, double time, double opacity)
{
    void *pix;
    int pitch;
    SDL_LockTexture(this->tex, NULL, &pix, &pitch);

    int i, j;
    for (i = 0; i < H; ++i)
        for (j = 0; j < W; ++j) {
            float o = (this->val[i * W + j] - time) / NEIGHBOUR;
            o = (o < 0 ? 0 : (o > 1 ? 1 : o)) * opacity;
            *(Uint32 *)(pix + i * pitch + j * 4) = 0xffffff00 | round(o * 255);
        }

    SDL_UnlockTexture(this->tex);
    SDL_RenderCopy(g_renderer, this->tex, NULL, NULL);
}
