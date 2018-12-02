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
static const double D4 = 1;
static const double T3 = 0.7;

/**
 *   \  | delay 1 |   fade   | delay2 |   txt1  | delay3 |    txt2    | delay4 |  txt3  |
 * Gmpl |         | cam move |        |         |        |            |        |        |
 * Imag |         |          |        |         |        | show, move |        |        |
 * Unvl |         |  fade in |        |         |        |   unveil   |        |        |
 * Text |         |          |        | fade in |        | fade, tint |        | result |
 * Line |         |          |        |       extend     |    tint    |        |        |
 **/

static const double CAM_MOVE = 4;
static const double LINE_X = 0.15;
static const double LINE_Y = 0.4;
static const double LINE_WIDTH = 4;

static void chapfin_tick(chapfin_scene *this, double dt)
{
    scene_tick((scene *)this->g, dt);
    this->time += dt;
}

static void chapfin_draw(chapfin_scene *this)
{
    double u_opacity = 0;
    if (this->time < D1 + F + D2) {
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
    } else if (this->time < D1 + F + D2 + T1 + D3 + T2 + D4) {
        double r = (this->time - (D1 + F + D2 + T1 + D3)) / T2;
        if (r > 1) r = 1;
        double s = (this->time - (D1 + F + D2 + T1 + D3)) / T2_TEXT_DUR;
        if (s > 1) s = 1;
        SDL_SetRenderDrawColor(g_renderer, 255, 192, 192, 255);
        SDL_RenderClear(g_renderer);
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
        double r = (this->time - (D1 + F + D2 + T1 + D3 + T2 + D4)) / T3;
        if (r > 1) r = 1;
        SDL_SetRenderDrawColor(g_renderer, 255, 192, 192, 255);
        SDL_RenderClear(g_renderer);
        label_colour_mod(this->l_num, 255, 255, 255);
        label_colour_mod(this->l_title, 255, 255, 255);
        this->l_extra->_base.alpha = round(r * 255);
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
    if (this->time < D1 + F + D2 + T1 + D3 + T2 + D4) return;
    g_stage = (scene *)loading_create(
        &g_stage, (loading_routine)exit_routine,
        (loading_postroutine)exit_postroutine, this);
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
        (SDL_Color){255, 255, 255}, WIN_W, "14:46:40.412");
    element_place_anchored((element *)l, WIN_W / 2, WIN_H * 0.8, 0.5, 0.5);
    l->_base.alpha = 0;
    bekter_pushback(this->_base.children, l);
    this->l_extra = l;

    g->hor_state = HOR_STATE_NONE;
    g->ver_state = VER_STATE_NONE;

    return this;
}
