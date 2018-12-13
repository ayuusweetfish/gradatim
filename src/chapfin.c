#include "chapfin.h"

#include "global.h"
#include "loading.h"
#include "profile_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
static const double LINE_Y = 0.387;
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
    if (s == NULL || t < 0) return;
    double r = (t > T3 ? 1 : t / T3);
    r = 1 - (1 - r) * (1 - r) * (1 - r) * (1 - r);
    /*r = ease_elastic_out(r, 0.9);*/
    element_place((element *)s, iround(WIN_W * x), iround(WIN_H * (y + 0.1 * (1 - r))));
    s->alpha = iround(r * 255);
}

static void chapfin_draw(chapfin_scene *this)
{
    double u_opacity = 0;
    if (this->summary_shown) {
        render_texture(this->bg_tex, NULL);
        floue_draw(this->f);
        int i;
        for (i = 0; i < N_MODS; ++i)
            update_summary_obj((sprite *)this->mod_icon[i],
                ICON_BASE_X - ICON_SZ * (N_MODS - i), 0.7, this->time - I3 * i);
        update_summary_obj((sprite *)this->l_summary,
            ICON_BASE_X, 0.7, this->time - I3 * N_MODS);
        update_summary_obj((sprite *)this->clock,
            ICON_BASE_X - ICON_SZ, 0.77, this->time - I3 * (N_MODS + 1));
        if (this->l_timer != NULL) {
            update_summary_obj((sprite *)this->l_timer,
                ICON_BASE_X, 0.77, this->time - I3 * (N_MODS + 2));
            update_summary_obj((sprite *)this->l_timer_d,
                (double)(this->l_timer->_base._base.dim.x + this->l_timer->_base._base.dim.w) / WIN_W,
                0.77, this->time - I3 * (N_MODS + 3));
        }
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
        this->l_num->_base.alpha = iround(r * 255);
        double x = 0.5 - ease_quad_inout(s) * (0.5 - LINE_X);
        SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, iround(r * 255));
        SDL_RenderFillRect(g_renderer, &(SDL_Rect){
            WIN_W * x, WIN_H * LINE_Y,
            WIN_W * (1 - x * 2), LINE_WIDTH
        });
    } else if (this->time < D1 + F + D2 + T1 + D3 + T2) {
        double r = (this->time - (D1 + F + D2 + T1 + D3)) / T2;
        if (r > 1) r = 1;
        double s = (this->time - (D1 + F + D2 + T1 + D3)) / T2_TEXT_DUR;
        if (s > 1) s = 1;
        render_texture(this->bg_tex, NULL);
        floue_draw(this->f);
        unveil_draw(this->u, r, 1);
        int grey = iround(s * 255);
        label_colour_mod(this->l_num, grey, grey, grey);
        label_colour_mod(this->l_title, grey, grey, grey);
        this->l_title->_base.alpha = iround(s * 255);
        SDL_SetRenderDrawColor(g_renderer, grey, grey, grey, 255);
        SDL_RenderFillRect(g_renderer, &(SDL_Rect){
            WIN_W * LINE_X, WIN_H * LINE_Y,
            WIN_W * (1 - LINE_X * 2), LINE_WIDTH
        });
    } else {
        render_texture(this->bg_tex, NULL);
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
    if (!this->summary_shown) {
        if (this->time >= D1 + F + D2 + T1 + D3 + T2) {
            this->summary_shown = true;
            this->time = 0;
        }
    } else {
        g_stage = (scene *)loading_create(
            &g_stage, (loading_routine)exit_routine,
            (loading_postroutine)exit_postroutine, this);
    }
}

static inline void set_time(chapfin_scene *this, int t)
{
    sprite *sp = sprite_create("clock.png");
    sp->_base.dim.w = sp->_base.dim.h = WIN_W * ICON_REAL_SZ;
    sp->alpha = 0;
    bekter_pushback(this->_base.children, sp);
    this->clock = sp;

    label *l = label_create(FONT_UPRIGHT, 28,
        (SDL_Color){255, 255, 255}, WIN_W, "");
    l->_base.alpha = 0;
    bekter_pushback(this->_base.children, l);
    this->l_timer = l;

    l = label_create(FONT_UPRIGHT, 21,
        (SDL_Color){255, 255, 255}, WIN_W, "");
    l->_base.alpha = 0;
    bekter_pushback(this->_base.children, l);
    this->l_timer_d = l;

    char s[16], d[8];
    t *= this->g->chap->beat_mul;
    int sig = this->g->chap->sig;
    sprintf(s, "%03d. %02d. ", t / (sig * 48), t % (sig * 48) / 48);
    sprintf(d, "%02d", t % 48);
    label_set_text(this->l_timer, s);
    label_set_text(this->l_timer_d, d);
}

static inline void set_retries(chapfin_scene *this, int n)
{
    sprite *sp = sprite_create("retry_count.png");
    sp->_base.dim.w = sp->_base.dim.h = WIN_W * ICON_REAL_SZ;
    sp->alpha = 0;
    bekter_pushback(this->_base.children, sp);
    this->mushroom = sp;

    char s[16];
    sprintf(s, "%d", n);
    label *l = label_create(FONT_UPRIGHT, 28,
        (SDL_Color){255, 255, 255}, WIN_W, s);
    l->_base.alpha = 0;
    bekter_pushback(this->_base.children, l);
    this->l_retries = l;
}

chapfin_scene *chapfin_scene_create(gameplay_scene *g)
{
    chapfin_scene *this = malloc(sizeof(chapfin_scene));
    memset(this, 0, sizeof(chapfin_scene));
    this->_base.children = bekter_create();
    this->_base.tick = (scene_tick_func)chapfin_tick;
    this->_base.draw = (scene_draw_func)chapfin_draw;
    this->_base.drop = (scene_drop_func)chapfin_drop;
    this->_base.key_handler = (scene_key_func)chapfin_key;
    this->g = g;
    this->time = 0;
    this->orig_cam_y = g->cam_y;
    this->bg_tex = retrieve_texture(g->chap->endgame_img);
    this->u = unveil_create();
    this->f = floue_create((SDL_Color){0});
    this->summary_shown = false;

    int i;
    for (i = 0; i < 6; ++i)
        floue_add(this->f, (SDL_Point){rand() % WIN_W, rand() % WIN_H},
            (SDL_Color){g->chap->r1, g->chap->g1, g->chap->b1, 255},
            rand() % (WIN_W / 4) + WIN_W / 4, (double)(i + 1) / 12);

    char s[64];
    if (g->chap->idx == 0) sprintf(s, "P r e l u d e");
    else sprintf(s, "C h a p t e r  %d", g->chap->idx);
    label *l = label_create(FONT_UPRIGHT, 40,
        (SDL_Color){255, 255, 255}, WIN_W, s);
    element_place_anchored((element *)l, WIN_W / 2, WIN_H * 0.3, 0.5, 0.5);
    l->_base.alpha = 0;
    bekter_pushback(this->_base.children, l);
    this->l_num = l;

    int p = 0;
    for (; g->chap->title[p] != '\0'; ++p) {
        s[p * 2] = g->chap->title[p];
        s[p * 2 + 1] = ' ';
    }
    s[p * 2 - 1] = '\0';
    l = label_create(FONT_UPRIGHT, 72,
        (SDL_Color){255, 255, 255}, WIN_W, s);
    element_place_anchored((element *)l, WIN_W / 2, WIN_H * 0.5, 0.5, 0.5);
    l->_base.alpha = 0;
    bekter_pushback(this->_base.children, l);
    this->l_title = l;

    l = label_create(FONT_UPRIGHT, 28,
        (SDL_Color){255, 255, 255}, WIN_W, "");
    l->_base.alpha = 0;
    bekter_pushback(this->_base.children, l);
    this->l_summary = l;

    for (i = 0; i < N_MODS; ++i) {
        int idx = (this->g->mods >> (i * 2)) & 3;
        sprite *sp = sprite_create(MODS[i][idx > 2 ? 2 : idx].icon);
        sp->_base.dim.w = sp->_base.dim.h = WIN_W * ICON_REAL_SZ;
        sp->alpha = 0;
        bekter_pushback(this->_base.children, sp);
        this->mod_icon[i] = sp;
    }

    /* Calculate summary */
    if (this->g->start_stage_idx == 0) {
        /* (1) Complete runthrough */
        label_set_text(this->l_summary, "Runthrough");
        set_time(this, (int)(this->g->total_time * 48));
        set_retries(this, this->g->retry_count);
    } else {
        /* (2) All cleared */
        /* (3) x/x stages cleared */
        int ct = 0, total_t = 0, t;
        int cmbid = modcomb_id(this->g->mods);
        for (i = 0; i < this->g->chap->n_stages; ++i) {
            if ((t = profile_get_stage(this->g->chap->idx, i)->time[cmbid]) != -1) {
                ++ct;
                total_t += t;
            }
        }
        if (ct == this->g->chap->n_stages) {
            label_set_text(this->l_summary, "All cleared");
            set_time(this, total_t);
        } else {
            sprintf(s, "%d/%d stages cleared", ct, this->g->chap->n_stages);
            label_set_text(this->l_summary, s);
        }
    }

    return this;
}
