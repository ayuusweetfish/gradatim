/* The main menu */
/* The word `main` is deliberately avoided */

#ifndef _COUVERTURE_H
#define _COUVERTURE_H

#include "scene.h"
#include "floue.h"

#include <stdbool.h>

typedef struct _couverture {
    scene _base;
    floue *f;
    int r1, g1, b1, r2, g2, b2;
    bool finished;
} couverture;

couverture *couverture_create();

#endif
