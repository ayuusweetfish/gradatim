/* Resource management */

#ifndef _RESOURCES_H
#define _RESOURCES_H

#include "bekter.h"

#include <SDL.h>
#include <SDL_ttf.h>

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
void render_texture_ex(texture t, SDL_Rect *dim,
    double a, SDL_Point *c, SDL_RendererFlip f);
void render_texture_scaled(texture t, double x, double y, double scale);
void render_texture_alpha(texture t, SDL_Rect *dim, int alpha);

#define FONT_ITALIC     0
#define FONT_UPRIGHT    1
#define FONT_OUTLINE_START      2
#define FONT_UPRIGHT_OUTLINE    2

TTF_Font *load_font(int id, int pts);

#endif
