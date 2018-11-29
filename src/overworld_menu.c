#include "overworld_menu.h"
#include "global.h"

static const double MOV_X = 30;
static const double SCALE = 1.3;

static const double FADEIN_DUR = 0.3;
static const double MENU_W = WIN_W / 3;

static inline void owm_tick(overworld_menu *this, double dt)
{
    scene_tick(&this->bg->_base, dt);
    this->time += dt;
    if (this->quit_time >= 0 && this->time >= this->quit_time + FADEIN_DUR) {
        g_stage = (scene *)this->bg;
        scene_drop(this);
    }
}

static inline void owm_draw(overworld_menu *this)
{
    scene_draw(&this->bg->_base);

    double t;
    if (this->time < FADEIN_DUR) {
        t = this->time / FADEIN_DUR;
        t = 1 - (1 - t) * (1 - t) * (1 - t);
    } else if (this->time < this->quit_time + FADEIN_DUR) {
        t = (this->time - this->quit_time) / FADEIN_DUR;
        t = 1 - t * t * t;
    } else {
        t = 1;
    }
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, round(t * 192));
    SDL_RenderFillRect(g_renderer, &(SDL_Rect){
        round(WIN_W - MENU_W * t), 0, MENU_W, WIN_H
    });
}

static inline void owm_drop(overworld_menu *this)
{
}

static inline void owm_key(overworld_menu *this, SDL_KeyboardEvent *ev)
{
    if (ev->state != SDL_PRESSED) return;
    switch (ev->keysym.sym) {
        case SDLK_ESCAPE:
            if (this->quit_time < 0) {
                this->quit_time = this->time;
                this->bg->cam_targx -= MOV_X;
                this->bg->cam_targscale /= SCALE;
            }
            break;
    }
}

overworld_menu *overworld_menu_create(overworld_scene *bg)
{
    overworld_menu *ret = malloc(sizeof(overworld_menu));
    ret->_base.children = bekter_create();
    ret->_base.tick = (scene_tick_func)owm_tick;
    ret->_base.draw = (scene_draw_func)owm_draw;
    ret->_base.drop = (scene_drop_func)owm_drop;
    ret->_base.key_handler = (scene_key_func)owm_key;
    ret->bg = bg;
    ret->time = 0;
    ret->quit_time = -1;

    bg->cam_targx += MOV_X;
    bg->cam_targscale *= SCALE;

    return ret;
}
