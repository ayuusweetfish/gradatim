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
} couverture;

couverture *couverture_create();

#endif