/* Resource management */

#ifndef _RESOURCES_H
#define _RESOURCES_H

#include "bekter.h"

#include <SDL.h>

#define RES_HASH_SZ 997

typedef struct _texture {
    SDL_Texture *sdl_tex;
    SDL_Rect range;
} texture;

typedef struct _texture_kvpair {
    char *name;
    texture value;
} _texture_kvpair;

void load_images();
void release_images();
texture retrieve_texture(const char *name);
texture temp_texture(SDL_Texture *sdl_tex);
void render_texture(texture t, SDL_Rect *dim);
void render_texture_scaled(texture t, double x, double y, double scale);
void render_texture_alpha(texture t, SDL_Rect *dim, int alpha);

#endif
