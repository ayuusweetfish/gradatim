/* Global configurations */

#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <SDL.h>
#include <SDL_ttf.h>

#include "orion/orion.h"
#include "scene.h"

#define WIN_W   1080
#define WIN_H   720

#define TRACKID_MAIN_BGM    0
#define TRACKID_STAGE_BGM   8

#define iround(__x)  ((int)((__x) + 0.5))
#define sqr(__x) ((__x) * (__x))
double clamp(double x, double l, double u);

#define ease_quad_inout(__p) \
    ((__p) < 0.5 ? 2 * sqr(__p) : 1 - 2 * sqr(1 - (__p)))
double ease_elastic_out(double t, double p);

extern struct orion g_orion;
extern SDL_Window *g_window;
extern SDL_Renderer *g_renderer;
extern scene *g_stage;

SDL_Texture *load_texture(const char *path, int *w, int *h);

#endif
