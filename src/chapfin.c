#include "chapfin.h"

#include "global.h"
#include "loading.h"

static const double D1 = 0.5;
static const double F = 0.7;
static const double D2 = 1.5;
static const double T1 = 0.7;
static const double D3 = 1;
static const double T2 = 1;
static const double T2_TEXT_DUR = 0.4;

static const double T3 = 0.5;
static const double I3 = 0.02;

/**
 *   \  | delay 1 |   fade   | delay2 |   txt1  | delay3 |    txt2    |
 * Gmpl |         | cam move |        |         |        |            |
 * Imag |         |          |        |         |        | show, move |
 * Unvl |         |  fade in |        |         |        |   unveil   |
 * Text |         |          |        | fade in |        | fade, tint |
 * Line |         |          |        |       extend     |    tint    |
 **/

static const double CAM_MOVE = 4;
static const double LINE_X = 0.15;
static const double LINE_Y = 0.4;
static const double LINE_WIDTH = 4;
static const double ICON_BASE_X = 0.75;
static const double ICON_SZ = 0.05;
static const double ICON_REAL_SZ = 0.04;

static void chapfin_tick(chapfin_scene *this, double dt)
{
    scene_tick((scene *)this->g, dt);
    floue_tick(this->f, dt);
    this->time += dt;
}

static inline void update_summary_obj(sprite *s, double x, double y, double t)
{
    if (t < 0) return;
    double r = (t > T3 ? 1 : t / T3);
    r = 1 - (1 - r) * (1 - r) * (1 - r) * (1 - r);
    /*r = ease_elastic_out(r, 0.9);*/
    element_place((element *)s, round(WIN_W * x), round(WIN_H * (y + 0.1 * (1 - r))));
    s->alpha = round(r * 255);
}

static void chapfin_draw(chapfin_scene *this)
{
    double u_opacity = 0;
    if (this->summary_shown) {
        SDL_SetRenderDrawColor(g_renderer, 255, 192, 192, 255);
        SDL_RenderClear(g_renderer);
        floue_draw(this->f);
        int i;
        for (i = 0; i < N_MODS; ++i)
            update_summary_obj((sprite *)this->mod_icon[i],
                ICON_BASE_X - ICON_SZ * (N_MODS - i), 0.7, this->time - I3 * i);
        update_summary_obj((sprite *)this->l_summary,
            ICON_BASE_X, 0.7, this->time - I3 * N_MODS);
        update_summary_obj((sprite *)this->clock,
            ICON_BASE_X - ICON_SZ, 0.77, this->time - I3 * (N_MODS + 1));
        update_summary_obj((sprite *)this->l_timer,
            ICON_BASE_X, 0.77, this->time - I3 * (N_MODS + 2));
        update_summary_obj((sprite *)this->l_timer_d,
            (double)(this->l_timer->_base._base.dim.x + this->l_timer->_base._base.dim.w) / WIN_W,
            0.77, this->time - I3 * (N_MODS + 3));
        update_summary_obj((sprite *)this->mushroom,
            ICON_BASE_X - ICON_SZ, 0.84, this->time - I3 * (N_MODS + 4));
        update_summary_obj((sprite *)this->l_retries,
            ICON_BASE_X, 0.84, this->time - I3 * (N_MODS + 5));
        SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(g_renderer, &(SDL_Rect){
            WIN_W * LINE_X, WIN_H * LINE_Y,
            WIN_W * (1 - LINE_X * 2), LINE_WIDTH
        });
    } else if (this->time < D1 + F + D2) {
        if (this->time >= D1 && this->time < D1 + F) {
            double r = (this->time - D1) / F;
            this->g->cam_y = this->orig_cam_y - r * r * CAM_MOVE;
            u_opacity = r;
        } else if (this->time >= D1 + F) {
            u_opacity = 1;
        }
        scene_draw((scene *)this->g);
        if (u_opacity != 0) unveil_draw(this->u, 0, u_opacity);
    } else if (this->time < D1 + F + D2 + T1 + D3) {
        double r = (this->time - (D1 + F + D2)) / T1;
        if (r > 1) r = 1;
        double s = (this->time - (D1 + F + D2)) / (T1 + D3);
        /* s is guaranteed to fall in [0, 1) */
        unveil_draw(this->u, 0, 1);
        label_colour_mod(this->l_num, 0, 0, 0);
        this->l_num->_base.alpha = round(r * 255);
        double x = 0.5 - ease_quad_inout(s) * (0.5 - LINE_X);
        SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, round(r * 255));
        SDL_RenderFillRect(g_renderer, &(SDL_Rect){
            WIN_W * x, WIN_H * LINE_Y,
            WIN_W * (1 - x * 2), LINE_WIDTH
        });
    } else if (this->time < D1 + F + D2 + T1 + D3 + T2) {
        double r = (this->time - (D1 + F + D2 + T1 + D3)) / T2;
        if (r > 1) r = 1;
        double s = (this->time - (D1 + F + D2 + T1 + D3)) / T2_TEXT_DUR;
        if (s > 1) s = 1;
        SDL_SetRenderDrawColor(g_renderer, 255, 192, 192, 255);
        SDL_RenderClear(g_renderer);
        floue_draw(this->f);
        unveil_draw(this->u, r, 1);
        int grey = round(s * 255);
        label_colour_mod(this->l_num, grey, grey, grey);
        label_colour_mod(this->l_title, grey, grey, grey);
        this->l_title->_base.alpha = round(s * 255);
        SDL_SetRenderDrawColor(g_renderer, grey, grey, grey, 255);
        SDL_RenderFillRect(g_renderer, &(SDL_Rect){
            WIN_W * LINE_X, WIN_H * LINE_Y,
            WIN_W * (1 - LINE_X * 2), LINE_WIDTH
        });
    } else {
        SDL_SetRenderDrawColor(g_renderer, 255, 192, 192, 255);
        SDL_RenderClear(g_renderer);
        floue_draw(this->f);
        label_colour_mod(this->l_num, 255, 255, 255);
        label_colour_mod(this->l_title, 255, 255, 255);
        SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(g_renderer, &(SDL_Rect){
            WIN_W * LINE_X, WIN_H * LINE_Y,
            WIN_W * (1 - LINE_X * 2), LINE_WIDTH
        });
    }
    scene_draw_children((scene *)this);
}

static void chapfin_drop(chapfin_scene *this)
{
    scene_drop(this->g);
    unveil_drop(this->u);
    floue_drop(this->f);
}

static scene *exit_routine(chapfin_scene *this)
{
    return this->g->bg;
}

static void exit_postroutine(chapfin_scene *this, void *unused)
{
    scene_drop(this);
}

static void chapfin_key(chapfin_scene *this, SDL_KeyboardEvent *ev)
{
    if (ev->state != SDL_PRESSED) return;
    if (this->time < D1 + F + D2 + T1 + D3 + T2) return;
    if (!this->summary_shown) {
        this->summary_shown = true;
        this->time = 0;
    } else {
        g_stage = (scene *)loading_create(
            &g_stage, (loading_routine)exit_routine,
            (loading_postroutine)exit_postroutine, this);
    }
}

chapfin_scene *chapfin_scene_create(gameplay_scene *g)
{
    chapfin_scene *this = malloc(sizeof(chapfin_scene));
    this->_base.children = bekter_create();
    this->_base.tick = (scene_tick_func)chapfin_tick;
    this->_base.draw = (scene_draw_func)chapfin_draw;
    this->_base.drop = (scene_drop_func)chapfin_drop;
    this->_base.key_handler = (scene_key_func)chapfin_key;
    this->g = g;
    this->time = 0;
    this->orig_cam_y = g->cam_y;
    this->u = unveil_create();
    this->f = floue_create((SDL_Color){0});
    this->summary_shown = false;

    int i;
    for (i = 0; i < 6; ++i)
        floue_add(this->f, (SDL_Point){rand() % WIN_W, rand() % WIN_H},
            (SDL_Color){g->chap->r1, g->chap->g1, g->chap->b1, 255},
            rand() % (WIN_W / 4) + WIN_W / 4, (double)(i + 1) / 12);

    char s[64];
    sprintf(s, "Chapter %d", g->chap->idx + 1);
    label *l = label_create("KiteOne-Regular.ttf", 40,
        (SDL_Color){255, 255, 255}, WIN_W, s);
    element_place_anchored((element *)l, WIN_W / 2, WIN_H * 0.3, 0.5, 0.5);
    l->_base.alpha = 0;
    bekter_pushback(this->_base.children, l);
    this->l_num = l;

    l = label_create("KiteOne-Regular.ttf", 72,
        (SDL_Color){255, 255, 255}, WIN_W, g->chap->title);
    element_place_anchored((element *)l, WIN_W / 2, WIN_H * 0.5, 0.5, 0.5);
    l->_base.alpha = 0;
    bekter_pushback(this->_base.children, l);
    this->l_title = l;

    l = label_create("KiteOne-Regular.ttf", 28,
        (SDL_Color){255, 255, 255}, WIN_W, "Complete run");
    l->_base.alpha = 0;
    bekter_pushback(this->_base.children, l);
    this->l_summary = l;

    l = label_create("KiteOne-Regular.ttf", 28,
        (SDL_Color){255, 255, 255}, WIN_W, "015.  07.");
    l->_base.alpha = 0;
    bekter_pushback(this->_base.children, l);
    this->l_timer = l;

    l = label_create("KiteOne-Regular.ttf", 21,
        (SDL_Color){255, 255, 255}, WIN_W, "47");
    l->_base.alpha = 0;
    bekter_pushback(this->_base.children, l);
    this->l_timer_d = l;

    l = label_create("KiteOne-Regular.ttf", 28,
        (SDL_Color){255, 255, 255}, WIN_W, "4073");
    l->_base.alpha = 0;
    bekter_pushback(this->_base.children, l);
    this->l_retries = l;

    sprite *sp = sprite_create("clock.png");
    sp->_base.dim.w = sp->_base.dim.h = WIN_W * ICON_REAL_SZ;
    sp->alpha = 0;
    bekter_pushback(this->_base.children, sp);
    this->clock = sp;

    sp = sprite_create("retry_count.png");
    sp->_base.dim.w = sp->_base.dim.h = WIN_W * ICON_REAL_SZ;
    sp->alpha = 0;
    bekter_pushback(this->_base.children, sp);
    this->mushroom = sp;

    for (i = 0; i < N_MODS; ++i) {
        int idx = (this->g->mods >> (i * 2)) & 3;
        sp = sprite_create(MODS[i][idx > 2 ? 2 : idx].icon);
        sp->_base.dim.w = sp->_base.dim.h = WIN_W * ICON_REAL_SZ;
        sp->alpha = 0;
        bekter_pushback(this->_base.children, sp);
        this->mod_icon[i] = sp;
    }

    return this;
}
