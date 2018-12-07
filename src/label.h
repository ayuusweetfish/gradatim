/* Text */

#ifndef _LABEL_H
#define _LABEL_H

#include "element.h"

#include <SDL_ttf.h>

typedef struct _label {
    sprite _base;
    TTF_Font *font, *font_outline;
    SDL_Color cl, cl_outline;
    const char *text;
    int wid;
    unsigned long last_hash;
} label;

label *label_create(int font_id, int pts,
    SDL_Color cl, int wid, const char *text);
label *label_create_outlined(int font_id, int font_outline_id, int pts,
    SDL_Color cl, SDL_Color cl_outline, int wid, const char *text);
void label_set_text(label *this, const char *text);
void label_set_keyed_text(label *this, const char *text, const char *keys);
void label_colour_mod(label *this, Uint8 r, Uint8 g, Uint8 b);

#endif
