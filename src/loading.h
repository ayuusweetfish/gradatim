/* Loading scene for any operation */

#ifndef _LOADING_H
#define _LOADING_H

#include "transition.h"

#include <SDL.h>
#include <stdbool.h>

typedef scene *(*loading_routine)(void *ptr);

typedef struct _loading_scene {
    transition_scene _base;

    loading_routine func;
    void *userdata;

    SDL_Thread *thread;
    SDL_SpinLock lock;
    double until_next_check;
    double since_finish;
} loading_scene;

loading_scene *loading_create(scene **a, loading_routine func, void *userdata);

#endif
