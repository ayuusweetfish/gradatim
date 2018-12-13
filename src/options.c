#include "options.h"
#include "global.h"
#include "profile_data.h"
#include "transition.h"
#include "label.h"
#include "orion/orion.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* Repeat yourself */
#define N_MENU OPTIONS_N_MENU
static const double ITEM_OFFSET_Y = 0.29375;
static const double ITEM_SKIP = 0.10625;
static const double ITEM_H = 0.1;

static const double MENU_TR_DUR = 0.15;
static const double BLINK_DUR = 0.75;

static const double BGM_VOL = 0.5;
static const double BGM_LP_VOL = 0.2;

static const char *MENU_TEXT[N_MENU] = {
    "Music volume", "Sound effects",
    "Speedrun clock", "Fullscreen", "Audio-video offset"
};
static const int MENU_MAX[N_MENU] = {VOL_RESOLUTION, VOL_RESOLUTION, 1, 1, 200};
static const int MENU_OFFS[N_MENU] = {0, 0, 0, 0, -100};
static const bool MENU_LOOPS[N_MENU] = {false, false, true, true, false};

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
    int opacity = 128 + iround(phase * 24);
    SDL_SetRenderDrawColor(g_renderer, 255, 255, 128, opacity);
    SDL_RenderFillRect(g_renderer, &(SDL_Rect){
        0, iround((cur_y - ITEM_H / 2) * WIN_H), WIN_W, WIN_H * ITEM_H
    });

    scene_draw_children((scene *)this);
}

static void options_drop(options_scene *this)
{
    floue_drop(this->f);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM, 0.2, profile.bgm_vol * VOL_VALUE);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM_LP, 0.2, 0);
}

static inline void update_label(options_scene *this, int idx)
{
    char s[8];
    if (MENU_MAX[idx] == 1)
        strcpy(s, this->menu_val[idx] ? "Yes" : "No");
    else if (idx == 0 || idx == 1)
        sprintf(s, "%d%%", this->menu_val[idx] * 100 / MENU_MAX[idx]);
    else
        sprintf(s, "%c%d ms", this->menu_val[idx] >= 0 ? '+' : '-',
            abs(this->menu_val[idx]));
    label_set_text(this->l_menuval[idx], s);
    element_place_anchored((element *)this->l_menuval[idx],
        WIN_W * 5 / 6, (ITEM_OFFSET_Y + idx * ITEM_SKIP) * WIN_H, 1, 0.5);
}

static inline void init_menu_val(options_scene *this)
{
    this->menu_val[0] = profile.bgm_vol;
    this->menu_val[1] = profile.sfx_vol;
    this->menu_val[2] = (int)profile.show_clock;
    this->menu_val[3] = (int)profile.fullscreen;
    this->menu_val[4] = profile.av_offset;
}

static inline void update_profile(options_scene *this)
{
    profile.bgm_vol = this->menu_val[0];
    profile.sfx_vol = this->menu_val[1];
    profile.show_clock = this->menu_val[2];
    profile.fullscreen = this->menu_val[3];
    profile.av_offset = this->menu_val[4];
    profile_save();
    orion_ramp(&g_orion, TRACKID_MAIN_BGM, 0.2, BGM_LP_VOL * profile.bgm_vol * VOL_VALUE);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM_LP, 0.2, BGM_VOL * profile.bgm_vol * VOL_VALUE);
    int i;
    for (i = TRACKID_FX_FIRST; i <= TRACKID_FX_LAST; ++i)
        orion_ramp(&g_orion, i, 0, profile.sfx_vol * VOL_VALUE);
    SDL_SetWindowFullscreen(g_window,
        profile.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
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
            orion_play_once(&g_orion, TRACKID_FX_SW1);
            break;
        case SDLK_DOWN:
            this->last_menu_idx = this->menu_idx;
            this->menu_idx = (this->menu_idx + 1) % N_MENU;
            this->menu_time = this->time;
            orion_play_once(&g_orion, TRACKID_FX_SW1);
            break;
        case SDLK_LEFT:
            this->menu_val[this->menu_idx] -= MENU_OFFS[this->menu_idx];
            if (this->menu_val[this->menu_idx] == 0) {
                this->menu_val[this->menu_idx] =
                    MENU_LOOPS[this->menu_idx] ? MENU_MAX[this->menu_idx] : 0;
            } else {
                this->menu_val[this->menu_idx]--;
            }
            this->menu_val[this->menu_idx] += MENU_OFFS[this->menu_idx];
            update_label(this, this->menu_idx);
            update_profile(this);
            orion_play_once(&g_orion, TRACKID_FX_SW2);
            break;
        case SDLK_RIGHT:
            this->menu_val[this->menu_idx] -= MENU_OFFS[this->menu_idx];
            if (this->menu_val[this->menu_idx] == MENU_MAX[this->menu_idx]) {
                this->menu_val[this->menu_idx] =
                    MENU_LOOPS[this->menu_idx] ? 0 : MENU_MAX[this->menu_idx];
            } else {
                this->menu_val[this->menu_idx]++;
            }
            this->menu_val[this->menu_idx] += MENU_OFFS[this->menu_idx];
            update_label(this, this->menu_idx);
            update_profile(this);
            orion_play_once(&g_orion, TRACKID_FX_SW2);
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

    init_menu_val(this);

    for (i = 0; i < N_MENU; ++i) {
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

    orion_ramp(&g_orion, TRACKID_MAIN_BGM, 0.2, BGM_LP_VOL * profile.bgm_vol * VOL_VALUE * 2 / 3);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM_LP, 0.2, BGM_VOL * profile.bgm_vol * VOL_VALUE);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM_CANON, 0.2, BGM_LP_VOL * profile.bgm_vol * VOL_VALUE / 3);

    return this;
}
