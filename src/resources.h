/* Resource management */

#ifndef _RESOURCES_H
#define _RESOURCES_H

#include "bekter.h"

#include <SDL.h>

#define RES_HASH_SZ 997

typedef struct _texture_kvpair {
    char *name;
    SDL_Texture *sdl_tex;
} _texture_kvpair;

extern bekter(_texture_kvpair) res_map[RES_HASH_SZ];

typedef struct _texture {
    SDL_Texture *sdl_tex;
    SDL_Rect range;
} texture;

void load_images();
void finalize_images();
texture retrieve_texture(const char *name);
texture temp_texture(const SDL_Texture *sdl_tex);
void render_texture(texture t, SDL_Rect *dim);

#endif
