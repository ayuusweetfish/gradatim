/* Overlay menu for the overworld */

#ifndef _OVERWORLD_MENU_H
#define _OVERWORLD_MENU_H

#include "overworld.h"
#include "element.h"
#include "label.h"
#include "mod.h"
#include "profile_data.h"

typedef struct _overworld_menu {
    scene _base;

    overworld_scene *bg;

    double time;
    double quit_time;

    profile_stage stg_rec;
    label *l_timer, *l_timer_dec;

    int menu_idx, last_menu_idx;
    double menu_time;
    sprite *mod_icon[N_MODS];
    label *mod_title[N_MODS];
    label *mod_desc[N_MODS];

    int menu_val[N_MODS];
} overworld_menu;

overworld_menu *overworld_menu_create(overworld_scene *bg);

#endif
