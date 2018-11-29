/* Overlay menu for the overworld */

#ifndef _OVERWORLD_MENU_H
#define _OVERWORLD_MENU_H

#include "overworld.h"

typedef struct _overworld_menu {
    scene _base;
    overworld_scene *bg;
    double time;
    double quit_time;
} overworld_menu;

overworld_menu *overworld_menu_create(overworld_scene *bg);

#endif
