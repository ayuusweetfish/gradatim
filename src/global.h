/* Global configurations */

#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <SDL.h>

#define WIN_W   1080
#define WIN_H   720

#define round(__x)  ((int)(__x + 0.5))

SDL_Texture *load_texture(SDL_Renderer *rdr, const char *path, int *w, int *h);

#endif
