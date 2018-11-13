/* Global configurations */

#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <SDL.h>
#include <SDL_ttf.h>

#include "scene.h"

#define WIN_W   1080
#define WIN_H   720

#define round(__x)  ((int)(__x + 0.5))

extern SDL_Window *g_window;
extern SDL_Renderer *g_renderer;
extern scene *g_stage;

SDL_Texture *load_texture(const char *path, int *w, int *h);
TTF_Font *load_font(const char *path, int pts);

#endif
