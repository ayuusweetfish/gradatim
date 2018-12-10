/* The main menu */
/* The word `main` is deliberately avoided */

#ifndef _COUVERTURE_H
#define _COUVERTURE_H

#include "scene.h"

typedef struct _couverture {
    scene _base;
} couverture;

couverture *couverture_create();

#endif
