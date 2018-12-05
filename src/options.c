#include "options.h"
#include "global.h"
#include "profile_data.h"
#include "transition.h"
#include "label.h"

#include <stdlib.h>
#include <stdbool.h>

/* Repeat yourself */
static const int N_MENU = OPTIONS_N_MENU;
static const double ITEM_OFFSET_Y = 1./3;
static const double ITEM_SKIP = 0.125;
static const double ITEM_H = 0.1;

static const double MENU_TR_DUR = 0.15;
static const double BLINK_DUR = 0.75;

static const char *MENU_TEXT[N_MENU] = {
    "Music volume", "SFX volume", "Speedrun clock", "Fullscreen"
};
static const int MENU_MAX[N_MENU] = {20, 20, 1, 1};
static const bool MENU_LOOPS[N_MENU] = {false, false, true, true};

static void options_tick(options_scene *this, double dt)
{
    floue_tick(this->f, dt);
    this->time += dt;
}

static void options_draw(options_scene *this)
{
    floue_draw(this->f);

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

static void options_drop(options_scene *this)
{
    floue_drop(this->f);
}

static inline void update_label(options_scene *this, int idx)
{
    char s[8];
    if (MENU_MAX[idx] == 1)
        strcpy(s, this->menu_val[idx] ? "Yes" : "No");
    else sprintf(s, "%d%%", this->menu_val[idx] * 100 / MENU_MAX[idx]);
    label_set_text(this->l_menuval[idx], s);
    element_place_anchored((element *)this->l_menuval[idx],
        WIN_W * 5 / 6, (ITEM_OFFSET_Y + idx * ITEM_SKIP) * WIN_H, 1, 0.5);
}

static void options_key(options_scene *this, SDL_KeyboardEvent *ev)
{
    if (ev->state != SDL_PRESSED) return;
    switch (ev->keysym.sym) {
        case SDLK_ESCAPE:
            g_stage = transition_slideup_create(&g_stage, this->bg, 0.5);
            break;
        case SDLK_UP:
            this->last_menu_idx = this->menu_idx;
            this->menu_idx = (this->menu_idx - 1 + N_MENU) % N_MENU;
            this->menu_time = this->time;
            break;
        case SDLK_DOWN:
            this->last_menu_idx = this->menu_idx;
            this->menu_idx = (this->menu_idx + 1) % N_MENU;
            this->menu_time = this->time;
            break;
        case SDLK_LEFT:
            if (this->menu_val[this->menu_idx] == 0) {
                this->menu_val[this->menu_idx] =
                    MENU_LOOPS[this->menu_idx] ? MENU_MAX[this->menu_idx] : 0;
            } else {
                this->menu_val[this->menu_idx]--;
            }
            update_label(this, this->menu_idx);
            break;
        case SDLK_RIGHT:
            if (this->menu_val[this->menu_idx] == MENU_MAX[this->menu_idx]) {
                this->menu_val[this->menu_idx] =
                    MENU_LOOPS[this->menu_idx] ? 0 : MENU_MAX[this->menu_idx];
            } else {
                this->menu_val[this->menu_idx]++;
            }
            update_label(this, this->menu_idx);
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

    this->time = this->menu_time = 0;
    this->menu_idx = this->last_menu_idx = 0;

    this->f = floue_create((SDL_Color){255, 216, 108, 255});
    int i;
    for (i = 0; i < 16; ++i)
        floue_add(this->f, (SDL_Point){rand() % WIN_W, rand() % WIN_H},
            (SDL_Color){255, i % 2 == 0 ? 255 : 128, i % 2 == 0 ? 64 : 0, 255},
            rand() % (WIN_W / 8) + WIN_W / 8,
            (double)rand() / RAND_MAX * 0.25 + 0.1);

    label *header = label_create(FONT_UPRIGHT, 60,
        (SDL_Color){0, 0, 0}, WIN_H, "OPTIONS");
    bekter_pushback(this->_base.children, header);
    element_place_anchored((element *)header, WIN_W - 24, WIN_H / 7, 1, 0.5);

    for (i = 0; i < N_MENU; ++i) {
        this->menu_val[i] = 0;
        label *l = label_create(FONT_UPRIGHT, 40,
            (SDL_Color){0, 0, 0}, WIN_W, MENU_TEXT[i]);
        bekter_pushback(this->_base.children, l);
        element_place_anchored((element *)l,
            WIN_W / 6, (ITEM_OFFSET_Y + i * ITEM_SKIP) * WIN_H, 0, 0.5);

        l = label_create(FONT_ITALIC, 40, (SDL_Color){0, 0, 0}, WIN_W, "");
        bekter_pushback(this->_base.children, l);
        this->l_menuval[i] = l;
        update_label(this, i);
    }

    label *footer = label_create(FONT_UPRIGHT, 32, (SDL_Color){0}, WIN_H, "");
    bekter_pushback(this->_base.children, footer);
    label_set_keyed_text(footer,
        " ..`.  ..`.  Select      ..`.  ..`.  Adjust      ..`.  Back", "^v<>~");
    element_place_anchored((element *)footer, WIN_W / 2, WIN_H * 6 / 7, 0.5, 0.5);

    return this;
}
