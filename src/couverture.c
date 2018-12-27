#include "couverture.h"
#include "global.h"
#include "options.h"
#include "credits.h"
#include "transition.h"
#include "profile_data.h"
#include "label.h"
#include "button.h"
#include "orion/orion.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define N_MENU  4
static const double MENU_TR_DUR = 0.2;  /* In seconds */
static const double MENU_PERSIST_DUR = 3;
static const int MENU_HL_W = WIN_H * 0.3;
static const int MENU_HL_Y = WIN_H * 0.65;

static const double TINT_DUR = 1./2;  /* In beats */
static const double POP_DUR = 1./3;

#define NC 4    /* Number of colours */
static const SDL_Color BG_C[NC] = {
    {245, 255, 234, 255}, {234, 245, 255, 255},
    {251, 234, 255, 255}, {255, 234, 234, 255}
};
static const SDL_Color DOT_C[NC + 1] = {
    {220, 240, 192, 255}, {192, 216, 255, 255},
    {234, 192, 255, 255}, {255, 192, 192, 255},
    {255, 210, 96, 255}
};

#define NG 3    /* Number of circle groups on floue */
#define GSZ 5   /* Number of circles in each group */
static const SDL_Point P[NG] = {
    {WIN_W * 0.25, WIN_H * 0.65},
    {WIN_W * 0.55, WIN_H * 0.35},
    {WIN_W * 0.85, WIN_H * 0.55}
};

static void couverture_tick(couverture *this, double dt)
{
    floue_tick(this->f, dt);
    this->time += dt;
}

static inline SDL_Color get_colour(const SDL_Color C[], int nc, int bar, double beat)
{
    if (beat >= 4 - TINT_DUR) {
        int r1 = C[bar].r, r2 = C[(bar + 1) % nc].r,
            g1 = C[bar].g, g2 = C[(bar + 1) % nc].g,
            b1 = C[bar].b, b2 = C[(bar + 1) % nc].b;
        double rate = (beat - (4 - TINT_DUR)) / TINT_DUR;
        return (SDL_Color){
            iround(r1 + rate * (r2 - r1)),
            iround(g1 + rate * (g2 - g1)),
            iround(b1 + rate * (b2 - b1)),
            255
        };
    } else {
        return C[bar];
    }
}

static inline double get_scale(double beat, double mul)
{
    if (beat < POP_DUR) {
        double rate = beat / POP_DUR;
        return 1 + mul * (1 - cos(M_PI * (1 + rate)));
    } else if (beat >= 4 - POP_DUR) {
        double rate = (beat - (4 - POP_DUR)) / POP_DUR;
        return 1 + mul * (1 - cos(M_PI * rate));
    } else {
        return 1;
    }
}

static void couverture_draw(couverture *this)
{
    double t = (orion_tell(&g_orion, TRACKID_MAIN_BGM) - BGM_LOOP_A) / 44100.0;

    if (t >= 0 && !this->canon_playing) {
        orion_play_loop(&g_orion, TRACKID_MAIN_BGM_CANON,
            BGM_LOOP_A + (-16 * BGM_BEAT + t) * 44100,
            BGM_LOOP_A, BGM_LOOP_B);
        orion_ramp(&g_orion, TRACKID_MAIN_BGM_CANON, 0, profile.bgm_vol * VOL_VALUE / 3);
        this->canon_playing = true;
    }

    t += BGM_BEAT * 40000;
    int bar = (int)(t / (BGM_BEAT * 4));
    double beat = t / BGM_BEAT - bar * 4;
    bar %= NC;

    double downbeat_scale = get_scale(beat, -0.04);
    SDL_Color downbeat_c = { 0 };
    if (bar == NC - 1 && beat >= 4 - TINT_DUR)
        downbeat_c = get_colour(DOT_C, NC + 1, NC, beat);

    this->f->c0 = get_colour(BG_C, NC, bar, beat);

    int i, j;
    for (i = 0; i < NG; ++i) {
        if ((beat += (1 - 0.03 * GSZ)) >= 4) {
            /* Do not take the modulo, as DOT_C[NC] will be needed */
            bar++;
            beat -= 4;
        }
        for (j = 0; j < GSZ; ++j) {
            if ((beat += 0.03) >= 4) {
                bar++;
                beat -= 4;
            }
            this->f->c[i * GSZ + j] = downbeat_c.a != 0 ?
                downbeat_c : get_colour(DOT_C, NC + 1, bar, beat);
            this->f->scale[i * GSZ + j] =
                downbeat_scale * get_scale(beat, 0.06);
        }
    }
    floue_draw(this->f);

    /* DRY? No. */
    /* Draw highlight */
    double cur_x = 0.23 + this->menu_idx * 0.18;
    int opacity = 255;
    if (this->time < this->menu_time + MENU_TR_DUR) {
        double p = (this->time - this->menu_time) / MENU_TR_DUR;
        if (this->menu_faded) opacity = iround(p * 255);
        p = 1 - (1 - p) * (1 - p) * (1 - p);
        double last_x = 0.23 + this->last_menu_idx * 0.18;
        cur_x = last_x + p * (cur_x - last_x);
    } else if (this->time > this->menu_time + MENU_PERSIST_DUR) {
        double p = (this->time - (this->menu_time + MENU_PERSIST_DUR)) / MENU_TR_DUR;
        opacity = p < 1 ? iround(255 * (1 - p)) : 0;
        this->menu_faded = true;
    } else {
        this->menu_faded = false;
    }
    SDL_SetTextureColorMod(this->f->tex[0], 255, 255, 192);
    SDL_SetTextureAlphaMod(this->f->tex[0], opacity);
    for (i = 0; i < 4; ++i)
        SDL_RenderCopy(g_renderer, this->f->tex[0], NULL, &(SDL_Rect){
            iround(cur_x * WIN_W) - MENU_HL_W / 2, MENU_HL_Y - MENU_HL_W / 2,
            MENU_HL_W, MENU_HL_W
        });

    scene_draw_children((scene *)this);
}

static void couverture_drop(couverture *this)
{
    floue_drop(this->f);
}

static inline void move_menu_highlight(couverture *this, int idx)
{
    if (this->menu_idx != idx) {
        this->last_menu_idx = this->menu_idx;
        this->menu_idx = idx;
        this->menu_time = this->time;
    }
}

static void options_cb(couverture *this)
{
    move_menu_highlight(this, 0);
    g_stage = transition_slidedown_create(&g_stage,
        (scene *)options_create((scene *)this), 0.5);
    ((transition_scene *)g_stage)->preserves_a = true;
}

static void credits_cb(couverture *this)
{
    move_menu_highlight(this, 1);
    g_stage = transition_slideup_create(&g_stage,
        (scene *)credits_create((scene *)this), 0.5);
    ((transition_scene *)g_stage)->preserves_a = true;
}

static void start_cb(couverture *this)
{
    move_menu_highlight(this, 2);
    g_stage = transition_slidedown_create(&g_stage, (scene *)this->ow, 0.5);
    ((transition_scene *)g_stage)->preserves_a = true;
}

static void quit_cb(couverture *this)
{
    move_menu_highlight(this, 3);
    SDL_PushEvent(&(SDL_Event){SDL_QUIT});
}

static void couverture_key(couverture *this, SDL_KeyboardEvent *ev)
{
    if (ev->state != SDL_PRESSED) return;

    typedef void (*couverture_cb)(couverture *);
    static const couverture_cb cbs[N_MENU] = {
        options_cb, credits_cb, start_cb, quit_cb
    };

    switch (ev->keysym.sym) {
        case SDLK_UP:
        case SDLK_LEFT:
            move_menu_highlight(this, (this->menu_idx - 1 + N_MENU) % N_MENU);
            orion_play_once(&g_orion, TRACKID_FX_SW1);
            break;
        case SDLK_DOWN:
        case SDLK_RIGHT:
            move_menu_highlight(this, (this->menu_idx + 1) % N_MENU);
            orion_play_once(&g_orion, TRACKID_FX_SW1);
            break;
        case SDLK_RETURN:
        case SDLK_SPACE:
        case SDLK_c:
        case SDLK_z:
            if (!this->menu_faded) cbs[this->menu_idx](this);
    }
}

void couverture_generate_dots(couverture *this)
{
    int i, j;
    for (i = 0; i < NG; ++i)
        for (j = 0; j < GSZ; ++j)
            floue_add(this->f,
                (SDL_Point){P[i].x + rand() % 401 - 200, P[i].y + rand() % 401 - 200},
                (SDL_Color){255, 255, 255}, 120, 0.5
            );

    this->ow = overworld_create((scene *)this);

    label *title = label_create(FONT_UPRIGHT, 72,
        (SDL_Color){0}, WIN_W, "G  R  A  D  A  T  I  M");
    element_place_anchored((element *)title, WIN_W / 2, WIN_H * 0.35, 0.5, 0.5);
    bekter_pushback(this->_base.children, title);

    button *options = button_create((button_callback)options_cb,
        this, "options_btn_2.png", "options_btn_2.png", "options_btn_2.png", 1.03, 0.98);
    element_place_anchored((element *)options, WIN_W * 0.23, WIN_H * 0.6, 0.5, 0.5);
    bekter_pushback(this->_base.children, options);

    button *credits = button_create((button_callback)credits_cb,
        this, "credits_btn_2.png", "credits_btn_2.png", "credits_btn_2.png", 1.03, 0.98);
    element_place_anchored((element *)credits, WIN_W * 0.41, WIN_H * 0.6, 0.5, 0.5);
    bekter_pushback(this->_base.children, credits);

    button *start = button_create((button_callback)start_cb,
        this, "run_btn.png", "run_btn.png", "run_btn.png", 1.03, 0.98);
    element_place_anchored((element *)start, WIN_W * 0.59, WIN_H * 0.6, 0.5, 0.5);
    bekter_pushback(this->_base.children, start);

    button *quit = button_create((button_callback)quit_cb,
        this, "quit_btn.png", "quit_btn.png", "quit_btn.png", 1.03, 0.98);
    element_place_anchored((element *)quit, WIN_W * 0.77, WIN_H * 0.6, 0.5, 0.5);
    bekter_pushback(this->_base.children, quit);

    label *l = label_create(FONT_UPRIGHT, 36, (SDL_Color){0}, WIN_W, "Options");
    element_place_anchored((element *)l, WIN_W * 0.23, WIN_H * 0.725, 0.5, 0.5);
    bekter_pushback(this->_base.children, l);

    l = label_create(FONT_UPRIGHT, 36, (SDL_Color){0}, WIN_W, "Credits");
    element_place_anchored((element *)l, WIN_W * 0.41, WIN_H * 0.725, 0.5, 0.5);
    bekter_pushback(this->_base.children, l);

    l = label_create(FONT_UPRIGHT, 36, (SDL_Color){0}, WIN_W, "Run");
    element_place_anchored((element *)l, WIN_W * 0.59, WIN_H * 0.725, 0.5, 0.5);
    bekter_pushback(this->_base.children, l);

    l = label_create(FONT_UPRIGHT, 36, (SDL_Color){0}, WIN_W, "Quit");
    element_place_anchored((element *)l, WIN_W * 0.77, WIN_H * 0.725, 0.5, 0.5);
    bekter_pushback(this->_base.children, l);
}

couverture *couverture_create()
{
    couverture *this = malloc(sizeof(couverture));
    memset(this, 0, sizeof(*this));
    this->_base.children = bekter_create();
    this->_base.tick = (scene_tick_func)couverture_tick;
    this->_base.draw = (scene_draw_func)couverture_draw;
    this->_base.drop = (scene_drop_func)couverture_drop;
    this->_base.key_handler = (scene_key_func)couverture_key;

    this->f = floue_create((SDL_Color){255, 255, 255, 216});

    orion_load_ogg(&g_orion, TRACKID_MAIN_BGM, "copycat.ogg");
    orion_play_loop(&g_orion, TRACKID_MAIN_BGM, 0, BGM_LOOP_A, BGM_LOOP_B);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM, 0, profile.bgm_vol * VOL_VALUE * 2 / 3);

    orion_apply_lowpass(&g_orion, TRACKID_MAIN_BGM, TRACKID_MAIN_BGM_LP, 1760);
    orion_play_loop(&g_orion, TRACKID_MAIN_BGM_LP, 0, BGM_LOOP_A, BGM_LOOP_B);
    orion_ramp(&g_orion, TRACKID_MAIN_BGM_LP, 0, 0);

    orion_apply_lowpass(&g_orion, TRACKID_MAIN_BGM, TRACKID_MAIN_BGM_CANON, 22050);

    orion_load_ogg(&g_orion, TRACKID_FX_SW1, "switch36.ogg");
    orion_load_ogg(&g_orion, TRACKID_FX_SW2, "switch12.ogg");
    orion_load_ogg(&g_orion, TRACKID_MENU_OPEN, "Menu1A.ogg");
    orion_load_ogg(&g_orion, TRACKID_MENU_CLOSE, "Menu1B.ogg");
    orion_load_ogg(&g_orion, TRACKID_MENU_CONFIRM, "Item2A.ogg");

    int i;
    for (i = TRACKID_FX_FIRST; i <= TRACKID_FX_LAST; ++i)
        orion_ramp(&g_orion, i, 0, profile.sfx_vol * VOL_VALUE);

    orion_overall_play(&g_orion);

    this->canon_playing = false;

    this->time = this->menu_time = 0;
    this->menu_idx = this->last_menu_idx = 2;
    this->menu_faded = true;

    return this;
}
