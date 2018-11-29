#include "overworld_menu.h"
#include "global.h"
#include "label.h"
#include "loading.h"
#include "gameplay.h"

static const double MOV_X = 30;
static const double SCALE = 1.3;

static const double FADEIN_DUR = 0.3;
static const double MENU_W = WIN_W * 0.382;

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

    int delta_x = round(MENU_W / 2 * (1 - t));
    SDL_RenderFillRect(g_renderer, &(SDL_Rect){
        round(WIN_W - MENU_W + delta_x), 0, WIN_W, WIN_H
    });

    int i; sprite *el;
    int opacity = round(t * 255);
    for bekter_each(this->_base.children, i, el) {
        el->alpha = opacity;
        el->_base.dim.x += delta_x;
    }
    scene_draw_children((scene *)this);
    for bekter_each(this->_base.children, i, el) {
        el->_base.dim.x -= delta_x;
    }
}

static inline void owm_drop(overworld_menu *this)
{
}

static inline scene *run_stage(overworld_scene *this)
{
    gameplay_scene *gp = gameplay_scene_create((scene *)this,
        bekter_at(this->chaps, 0, struct chap_rec *), this->cur_stage_idx);
    return (scene *)gp;
}

static inline void init_stage(overworld_scene *this, gameplay_scene *gp)
{
    gameplay_run_leadin(gp);
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
        case SDLK_RETURN:
            g_stage = (scene *)loading_create(&g_stage,
                (loading_routine)run_stage, (loading_postroutine)init_stage, this->bg);
            this->bg->cam_targx -= MOV_X;
            this->bg->cam_targscale /= SCALE;
            break;
    }
}

/* This is dumb and works only for 1~39, but who cares */
static inline void gen_roman_numeral(char *s, int n)
{
    while (n >= 10) {
        *(s++) = 'x';
        n -= 10;
    }
    if (n >= 5 && n <= 8) *(s++) = 'v';
    switch (n) {
        case 3: case 8: *(s++) = 'i';
        case 2: case 7: *(s++) = 'i';
        case 1: case 6: *(s++) = 'i';
        case 0: case 5: default: break;
        case 4: *(s++) = 'i'; *(s++) = 'v'; break;
        case 9: *(s++) = 'i'; *(s++) = 'x'; break;
    }
    *s = '\0';
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

    char s[64];
    strcpy(s, "Chapter X - ");
    s[8] = '1' + bg->cur_chap_idx;
    gen_roman_numeral(s + 12, bg->cur_stage_idx + 1);
    label *l = label_create("KiteOne-Regular.ttf", 40,
        (SDL_Color){255, 255, 255}, WIN_W, s);
    element_place_anchored((element *)l, WIN_W - 12, 12, 1, 0);
    bekter_pushback(ret->_base.children, l);

    sprite *sp = sprite_create("clock.png");
    sp->_base.dim.w /= 2;
    sp->_base.dim.h /= 2;
    element_place_anchored((element *)sp, WIN_W - MENU_W + 24, WIN_H * 0.25, 0, 0.5);
    bekter_pushback(ret->_base.children, sp);

    sp = sprite_create("retry_count.png");
    sp->_base.dim.w /= 2;
    sp->_base.dim.h /= 2;
    element_place_anchored((element *)sp, WIN_W - MENU_W + 24, WIN_H * 0.35, 0, 0.5);
    bekter_pushback(ret->_base.children, sp);

    l = label_create("KiteOne-Regular.ttf", 32,
        (SDL_Color){255, 255, 255}, WIN_W, "02:50:52");
    element_place_anchored((element *)l,
        WIN_W - MENU_W + WIN_W / 10, WIN_H * 0.25, 0, 0.5);
    bekter_pushback(ret->_base.children, l);

    l = label_create("KiteOne-Regular.ttf", 32,
        (SDL_Color){255, 255, 255}, WIN_W, "1665");
    element_place_anchored((element *)l,
        WIN_W - MENU_W + WIN_W / 10, WIN_H * 0.35, 0, 0.5);
    bekter_pushback(ret->_base.children, l);

    l = label_create("KiteOne-Regular.ttf", 40,
        (SDL_Color){255, 255, 255}, WIN_W, "Start (Enter)");
    element_place_anchored((element *)l,
        WIN_W - MENU_W / 2, WIN_H * 0.75, 0.5, 0.5);
    bekter_pushback(ret->_base.children, l);

    bg->cam_targx += MOV_X;
    bg->cam_targscale *= SCALE;

    return ret;
}
