/* Global configurations */

#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <SDL.h>
#include <SDL_ttf.h>

#include "orion/orion.h"
#include "scene.h"

#define WIN_W   1080
#define WIN_H   720

#define TRACKID_MAIN_BGM        0
#define TRACKID_MAIN_BGM_LP     1
#define TRACKID_MAIN_BGM_CANON  2
#define TRACKID_FX_FIRST        TRACKID_FX_SW1
#define TRACKID_FX_SW1          3
#define TRACKID_FX_SW2          4
#define TRACKID_MENU_OPEN       5
#define TRACKID_MENU_CLOSE      6
#define TRACKID_MENU_CONFIRM    7
#define TRACKID_FX_PICKUP       8
#define TRACKID_FX_HOP          9
#define TRACKID_FX_DASH         10
#define TRACKID_FX_UNAVAIL      11
#define TRACKID_FX_NXSTAGE      12
#define TRACKID_FX_FAIL         13
#define TRACKID_FX_LAST         TRACKID_MENU_CONFIRM
#define TRACKID_STAGE_BGM       14

#define BGM_LOOP_A  (5.85 * 44100)
#define BGM_LOOP_B  ((5.85 + 336 * 60.0 / 165) * 44100)
#define BGM_BEAT    (60.0 / 165)

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
