/* Pause scene */

#ifndef _PAUSE_H
#define _PAUSE_H

#include "scene.h"

typedef void (*retry_callback)(scene *a);

typedef struct _pause_scene {
    scene _base;
    scene *a, *b, **p;
    retry_callback cb;

    double time;
    int menu_idx, last_menu_idx;
    double menu_time;
} pause_scene;

/* `a` is the scene to Resume to, and `b` is the scene to Go Back to
 * When Go Back is selected, `a` will be dropped as well */
pause_scene *pause_scene_create(scene **a, retry_callback cb, scene *b);

#endif
