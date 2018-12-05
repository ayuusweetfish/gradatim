#include "options.h"
#include "global.h"
#include "profile_data.h"
#include "transition.h"

#include <stdlib.h>

static void options_tick(options_scene *this, double dt)
{
    floue_tick(this->f, dt);
    this->menu_time += dt;
}

static void options_draw(options_scene *this)
{
    floue_draw(this->f);
}

static void options_drop(options_scene *this)
{
    floue_drop(this->f);
}

static void options_key(options_scene *this, SDL_KeyboardEvent *ev)
{
    if (ev->state != SDL_PRESSED) return;
    switch (ev->keysym.sym) {
        case SDLK_ESCAPE:
            g_stage = transition_slideup_create(&g_stage, this->bg, 0.5);
            break;
    }
}

options_scene *options_create(scene *bg)
{
    options_scene *this = malloc(sizeof(options_scene));
    this->_base.children = bekter_create();
    this->_base.tick = (scene_tick_func)options_tick;
    this->_base.draw = (scene_draw_func)options_draw;
    this->_base.drop = (scene_drop_func)options_drop;
    this->_base.key_handler = (scene_key_func)options_key;
    this->bg = bg;

    this->f = floue_create((SDL_Color){255, 216, 108, 255});
    int i;
    for (i = 0; i < 16; ++i)
        floue_add(this->f, (SDL_Point){rand() % WIN_W, rand() % WIN_H},
            (SDL_Color){255, i % 2 == 0 ? 255 : 128, i % 2 == 0 ? 64 : 0, 255},
            rand() % (WIN_W / 8) + WIN_W / 8,
            (double)rand() / RAND_MAX * 0.25 + 0.1);

    return this;
}
