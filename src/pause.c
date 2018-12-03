#include "pause.h"

#include "global.h"
#include "label.h"
#include "loading.h"
#include "gameplay.h"

static const int N_MENU = 3;
static const double ITEM_OFFSET_Y = 0.425;
static const double ITEM_SKIP = 0.125;
static const double ITEM_H = 0.1;

static const double MENU_TR_DUR = 0.15;
static const double BLINK_DUR = 0.75;

static const char *MENU_TEXT[N_MENU] = {
    "Resume", "Restart stage", "Back to map"
};

static inline void pause_tick(pause_scene *this, double dt)
{
    if (this->time == 0) {
        /* XXX: Is this hack legitimate??? */
        ((gameplay_scene *)this->a)->paused = false;
        scene_tick(this->a, dt);
        ((gameplay_scene *)this->a)->paused = true;
    }
    this->time += dt;
}

static inline void pause_draw(pause_scene *this)
{
    scene_draw(this->a);
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 192);
    SDL_RenderFillRect(g_renderer, NULL);

    /* DRY? No. */
    /* Draw highlight */
    double cur_y = ITEM_OFFSET_Y + this->menu_idx * ITEM_SKIP;
    if (this->time < this->menu_time + MENU_TR_DUR) {
        double p = (this->time - this->menu_time) / MENU_TR_DUR;
        p = 1 - (1 - p) * (1 - p) * (1 - p);
        double last_y = ITEM_OFFSET_Y + this->last_menu_idx * ITEM_SKIP;
        cur_y = last_y + p * (cur_y - last_y);
    }
    double phase = fabs(BLINK_DUR - fmod(this->time, BLINK_DUR * 2)) / BLINK_DUR;
    phase = ease_quad_inout(phase);
    int opacity = 128 + round(phase * 24);
    SDL_SetRenderDrawColor(g_renderer, 255, 255, 128, opacity);
    SDL_RenderFillRect(g_renderer, &(SDL_Rect){
        0, round((cur_y - ITEM_H / 2) * WIN_H), WIN_W, WIN_H * ITEM_H
    });

    scene_draw_children((scene *)this);
}

static inline void resume(pause_scene *this)
{
    *(this->p) = this->a;
    scene_drop(this);
}

static scene *retry_routine(pause_scene *this)
{
    return this->a;
}

static void retry_postroutine(pause_scene *this, void *unused)
{
    this->cb(this->a);
    scene_drop(this);
}

static scene *quit_routine(pause_scene *this)
{
    return this->b;
}

static void quit_postroutine(pause_scene *this, void *unused)
{
    scene_drop(this->a);
    scene_drop(this);
}

static inline void retry(pause_scene *this)
{
    g_stage = (scene *)loading_create(
        &g_stage, (loading_routine)retry_routine,
        (loading_postroutine)retry_postroutine, this);
}

static inline void quit(pause_scene *this)
{
    g_stage = (scene *)loading_create(
        &g_stage, (loading_routine)quit_routine,
        (loading_postroutine)quit_postroutine, this);
}

static inline void pause_key_handler(pause_scene *this, SDL_KeyboardEvent *ev)
{
    if (ev->state != SDL_PRESSED) return;
    switch (ev->keysym.sym) {
        case SDLK_ESCAPE:
            resume(this);
            break;
        case SDLK_RETURN:
            switch (this->menu_idx) {
                case 0: resume(this); break;
                case 1: retry(this); break;
                case 2: quit(this); break;
            }
            break;
        case SDLK_UP:
        case SDLK_LEFT:
            this->last_menu_idx = this->menu_idx;
            this->menu_idx = (this->menu_idx - 1 + N_MENU) % N_MENU;
            this->menu_time = this->time;
            break;
        case SDLK_DOWN:
        case SDLK_RIGHT:
            this->last_menu_idx = this->menu_idx;
            this->menu_idx = (this->menu_idx + 1) % N_MENU;
            this->menu_time = this->time;
            break;
    }
}

pause_scene *pause_scene_create(scene **a, retry_callback cb, scene *b)
{
    pause_scene *ret = malloc(sizeof(pause_scene));
    ret->_base.children = bekter_create();
    ret->_base.tick = (scene_tick_func)pause_tick;
    ret->_base.draw = (scene_draw_func)pause_draw;
    ret->_base.drop = NULL;
    ret->_base.key_handler = (scene_key_func)pause_key_handler;
    ret->a = *a;
    ret->b = b;
    ret->p = a;
    ret->cb = cb;
    ret->menu_idx = ret->last_menu_idx = 0;
    ret->time = ret->menu_time = 0;

    label *header = label_create("KiteOne-Regular.ttf", 60,
        (SDL_Color){255, 255, 255}, WIN_H, "PAUSED");
    bekter_pushback(ret->_base.children, header);
    element_place_anchored((element *)header, WIN_W / 2, WIN_H / 4, 0.5, 0.5);

    int i;
    for (i = 0; i < N_MENU; ++i) {
        label *l = label_create("KiteOne-Regular.ttf", 48,
            (SDL_Color){255, 255, 255}, WIN_H, MENU_TEXT[i]);
        bekter_pushback(ret->_base.children, l);
        element_place_anchored((element *)l,
            WIN_W / 2, (ITEM_OFFSET_Y + i * ITEM_SKIP) * WIN_H, 0.5, 0.5);
    }

    return ret;
}
