/* Transitions that take one scene as the argument */

#ifndef _UNARY_TRANSITION_H
#define _UNARY_TRANSITION_H

#include "transition.h"

typedef void (*utransition_callback)(void *);

typedef struct _utransition {
    transition_scene _base;
    utransition_callback cb;
} utransition;

utransition *utransition_fade_create(
    scene **a, double dur, utransition_callback cb);

#endif
