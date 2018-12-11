/* The main menu */
/* The word `main` is deliberately avoided */

#ifndef _COUVERTURE_H
#define _COUVERTURE_H

#include "scene.h"
#include "floue.h"
#include "overworld.h"

#include <stdbool.h>

typedef struct _couverture {
    scene _base;

    floue *f;
    overworld_scene *ow;    /* Loaded at the start */

    bool canon_playing;
} couverture;

void couverture_generate_dots(couverture *this);
couverture *couverture_create();

#endif
