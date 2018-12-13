#include "unveil.h"
#include "global.h"

#include <stdlib.h>

#define SCALE 4
static const int W = WIN_W / SCALE;
static const int H = WIN_H / SCALE;

static const double NOISE_SCALE = 0.02;
static const float NEIGHBOUR = 0.1;

/* Ken Perlin's original permutation array */
#define PERM 151,160,137,91,90,15, \
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23, \
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33, \
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166, \
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244, \
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196, \
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123, \
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42, \
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9, \
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228, \
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107, \
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254, \
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
static const unsigned char perm[512] = {PERM, PERM};

/* Perlin noise implementation */
/* Reference: https://flafla2.github.io/2014/08/09/perlinnoise.html */
static inline double fade(double t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

static inline double grad(int hash, double x, double y)
{
    switch (hash & 7) {
        case 0: return +1 * x +2 * y;
        case 1: return +1 * x -2 * y;
        case 2: return -1 * x +2 * y;
        case 3: return -1 * x -2 * y;
        case 4: return +2 * x +1 * y;
        case 5: return +2 * x -1 * y;
        case 6: return -2 * x +1 * y;
        case 7: return -2 * x -1 * y;
    }
    return 0;   /* Unreachable */
}

static inline double lerp(double a, double b, double x)
{
    return a + x * (b - a);
}

static inline double perlin(double x, double y)
{
    int xi = (int)x & 255;
    int yi = (int)y & 255;
    double xf = x - (int)x;
    double yf = y - (int)y;
    double u = fade(xf);
    double v = fade(yf);

    int aa = perm[perm[xi] + yi];
    int ab = perm[perm[xi] + yi + 1];
    int ba = perm[perm[xi + 1] + yi];
    int bb = perm[perm[xi + 1] + yi + 1];

    double ret = lerp(
        lerp(grad(aa, xf, yf), grad(ba, xf - 1, yf), u),
        lerp(grad(ab, xf, yf - 1), grad(bb, xf - 1, yf - 1), u),
        v);
    return ret;
}

static inline float scale(float x, float a, float b, float c, float d)
{
    return (x - a) / (b - a) * (d - c) + c;
}

unveil *unveil_create()
{
    unveil *this = malloc(sizeof(unveil));
    this->tex = SDL_CreateTexture(g_renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, W, H);
    SDL_SetTextureBlendMode(this->tex, SDL_BLENDMODE_BLEND);

    this->val = malloc(W * H * sizeof(float));
    int i, j;
    int di = rand() % 1665, dj = rand() % 4073;
    float max = -1e10, min = 1e10;
    for (i = 0; i < H; ++i)
        for (j = 0; j < W; ++j) {
            float v = (float)perlin(
                1665 + di + i * NOISE_SCALE,
                4073 + dj + j * NOISE_SCALE);
            if (max < v) max = v;
            if (min > v) min = v;
        }
    for (i = 0; i < H; ++i)
        for (j = 0; j < W; ++j)
            this->val[i * W + j] = scale((float)perlin(
                1665 + di + i * NOISE_SCALE,
                4073 + dj + j * NOISE_SCALE
            ), min, max, NEIGHBOUR, 1 - NEIGHBOUR);

    return this;
}

void unveil_drop(unveil *this)
{
    SDL_DestroyTexture(this->tex);
    free(this->val);
    free(this);
}

void unveil_draw(unveil *this, double time, double opacity)
{
    time = (time < 0.5 ? 4 * time * time * time :
        1 - 4 * (1 - time) * (1 - time) * (1 - time));
    void *pix;
    int pitch;
    SDL_LockTexture(this->tex, NULL, &pix, &pitch);

    int i, j;
    for (i = 0; i < H; ++i)
        for (j = 0; j < W; ++j) {
            float o = (this->val[i * W + j] - time) / NEIGHBOUR;
            o = (o < 0 ? 0 : (o > 1 ? 1 : o)) * opacity;
            *(Uint32 *)(pix + i * pitch + j * 4) = 0xffffff00 | iround(o * 255);
        }

    SDL_UnlockTexture(this->tex);
    SDL_RenderCopy(g_renderer, this->tex, NULL, NULL);
}
