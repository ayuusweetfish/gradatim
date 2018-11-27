#include "overworld.h"
#include "global.h"

#include <stdlib.h>

static void ow_tick(overworld_scene *this, double dt)
{
}

static void ow_draw(overworld_scene *this)
{
}

static void ow_drop(overworld_scene *this)
{
    bekter_drop(this->chaps);
}

static void ow_key(overworld_scene *this, SDL_KeyboardEvent *ev)
{
    if (ev->state != SDL_PRESSED) return;
    switch (ev->keysym.sym) {
        case SDLK_ESCAPE:
            g_stage = this->bg;
            ow_drop(this);
            break;
    }
}

overworld_scene *overworld_create(scene *bg)
{
    overworld_scene *ret = malloc(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));
    ret->_base.tick = (scene_tick_func)ow_tick;
    ret->_base.draw = (scene_draw_func)ow_draw;
    ret->_base.drop = (scene_drop_func)ow_drop;
    ret->_base.key_handler = (scene_key_func)ow_key;
    ret->bg = bg;

    ret->chaps = bekter_create();

    return ret;
}
