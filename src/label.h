/* Text */

#ifndef _LABEL_H
#define _LABEL_H

#include "element.h"

#include <SDL_ttf.h>

typedef struct _label {
    sprite _base;
    TTF_Font *font;
    SDL_Color cl;
    const char *text;
} label;

element *label_create(const char *path, int pts, SDL_Color cl, const char *text);

#endif
