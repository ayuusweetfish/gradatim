/* Dialogue overlay */

#ifndef _DIALOGUE_H
#define _DIALOGUE_H

#include "bekter.h"
#include "scene.h"
#include "element.h"
#include "label.h"

typedef struct dialogue_entry {
    texture avatar;
    char *name;
    char *text;
    int text_len;
} dialogue_entry;

typedef struct _dialogue_scene {
    scene _base;
    scene *bg, **bg_ptr;
    bekter(dialogue_entry) script;

    SDL_Texture *bg_tex;
    int script_idx, script_len;
    double entry_lasted, last_tick;
    int last_textpos;

    /* Children for easy access */
    sprite *avatar_disp;
    label *name_disp;
    label *text_disp;
} dialogue_scene;

/* `script` will be taken care of by the scene and doesn't need to be freed */
dialogue_scene *dialogue_create(scene **bg, bekter(dialogue_entry) script);

#endif
