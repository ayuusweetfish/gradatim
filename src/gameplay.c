#include "gameplay.h"

#include "global.h"

#include <math.h>

static const double UNIT_PX = 48;
static const double WIN_W_UNITS = (double)WIN_W / UNIT_PX;
static const double WIN_H_UNITS = (double)WIN_H / UNIT_PX;

static const double BEAT = 60.0 / 132;  /* Temporary */
#define HOP_SPD SIM_GRAVITY
static const double HOP_PRED_DUR = 0.2;
static const double HOP_GRACE_DUR = 0.15;
#define ANTHOP_DELUGE_SPD (6.0 * SIM_GRAVITY)
static const double HOR_SPD = 2;

static inline double clamp(double x, double l, double u)
{
    return (x < l ? l : (x > u ? u : x));
}

static void gameplay_scene_tick(gameplay_scene *this, double dt)
{
    this->simulator->prot.vx = 
        (this->hor_state == HOR_STATE_LEFT) ? -HOR_SPD :
        (this->hor_state == HOR_STATE_RIGHT) ? +HOR_SPD : 0;
    this->simulator->prot.ay =
        (this->ver_state == VER_STATE_DOWN) ? 4.0 * SIM_GRAVITY : 0;
    if (this->mov_state == MOV_ANTHOP) {
        if ((this->mov_time -= dt / BEAT) <= 0) {
            /* Perform a jump */
            this->mov_state = MOV_NORMAL;
            this->simulator->prot.vy = -HOP_SPD;
        } else {
            /* Deluging */
            this->simulator->prot.ay = ANTHOP_DELUGE_SPD;
        }
    }

    double rt = this->rem_time + dt / BEAT;
    while (rt >= SIM_STEPLEN) {
        sim_tick(this->simulator);
        rt -= SIM_STEPLEN;
    }
    this->rem_time = rt;

    /* Move the camera */
    double dest_x = clamp(this->simulator->prot.x,
        WIN_W_UNITS / 2, this->simulator->gcols - WIN_W_UNITS / 2);
    double dest_y = clamp(this->simulator->prot.y,
        WIN_H_UNITS / 2, this->simulator->grows - WIN_H_UNITS / 2);
    double cam_dx = dest_x - (this->cam_x + WIN_W_UNITS / 2);
    double cam_dy = dest_y - (this->cam_y + WIN_H_UNITS / 2);
    double rate = (dt > 0.1 ? 0.1 : dt) * 10;
    this->cam_x += rate * cam_dx;
    this->cam_y += rate * cam_dy;
}

static void gameplay_scene_draw(gameplay_scene *this)
{
    SDL_SetRenderDrawColor(g_renderer, 216, 224, 255, 255);
    SDL_RenderClear(g_renderer);

    int rmin = floorf(this->cam_y),
        rmax = ceilf(this->cam_y + WIN_H_UNITS),
        cmin = floorf(this->cam_x),
        cmax = ceilf(this->cam_x + WIN_W_UNITS);
    int r, c;
    for (r = rmin; r < rmax; ++r)
        for (c = cmin; c < cmax; ++c) {
            sobj *o = &sim_grid(this->simulator, r, c);
            if (o->tag != 0) {
                render_texture(this->grid_tex[o->tag], &(SDL_Rect){
                    (c - this->cam_x) * UNIT_PX,
                    (r - this->cam_y) * UNIT_PX,
                    UNIT_PX, UNIT_PX
                });
            }
        }

    render_texture(this->prot_tex, &(SDL_Rect){
        (this->simulator->prot.x - this->cam_x) * UNIT_PX,
        (this->simulator->prot.y - this->cam_y) * UNIT_PX,
        round(this->simulator->prot.w * UNIT_PX),
        round(this->simulator->prot.h * UNIT_PX)
    });
}

static void gameplay_scene_drop(gameplay_scene *this)
{
    sim_drop(this->simulator);
}

static void try_hop(gameplay_scene *this)
{
    if ((this->simulator->cur_time - this->simulator->last_land) <= HOP_GRACE_DUR) {
        /* Grace jump */
        this->simulator->prot.vy = -HOP_SPD;
    } else if (sim_prophecy(this->simulator, HOP_PRED_DUR)) {
        /* Will land soon, deluge and then jump */
        this->mov_state = MOV_ANTHOP;
        /* Binary search on time needed till landing happens */
        double lo = 0, hi = HOP_PRED_DUR, mid;
        int i;
        for (i = 0; i < 10; ++i) {
            mid = (lo + hi) / 2;
            if (sim_prophecy(this->simulator, mid)) hi = mid;
            else lo = mid;
        }
        this->mov_time = hi;
    }
}

static void gameplay_scene_key_handler(gameplay_scene *this, SDL_KeyboardEvent *ev)
{
#define toggle(__thisstate, __keystate, __has, __none) do { \
    if ((__keystate) == SDL_PRESSED) (__thisstate) = (__has); \
    else if ((__thisstate) == (__has)) (__thisstate) = (__none); \
} while (0)

    if (ev->repeat) return;
    switch (ev->keysym.sym) {
        case SDLK_c:
            if (ev->state == SDL_PRESSED) try_hop(this);
            break;
        case SDLK_UP:
            toggle(this->ver_state, ev->state, VER_STATE_UP, VER_STATE_NONE);
            break;
        case SDLK_DOWN:
            toggle(this->ver_state, ev->state, VER_STATE_DOWN, VER_STATE_NONE);
            break;
        case SDLK_LEFT:
            toggle(this->hor_state, ev->state, HOR_STATE_LEFT, HOR_STATE_NONE);
            break;
        case SDLK_RIGHT:
            toggle(this->hor_state, ev->state, HOR_STATE_RIGHT, HOR_STATE_NONE);
            break;
        default: break;
    }
}

gameplay_scene *gameplay_scene_create(scene **bg)
{
    gameplay_scene *ret = malloc(sizeof(gameplay_scene));
    memset(ret, 0, sizeof(gameplay_scene));
    ret->_base.children = bekter_create();
    ret->_base.tick = (scene_tick_func)gameplay_scene_tick;
    ret->_base.draw = (scene_draw_func)gameplay_scene_draw;
    ret->_base.drop = (scene_drop_func)gameplay_scene_drop;
    ret->_base.key_handler = (scene_key_func)gameplay_scene_key_handler;
    ret->bg = *bg;
    ret->bg_ptr = bg;

    ret->simulator = sim_create(128, 128);
    ret->rem_time = 0;
    ret->prot_tex = retrieve_texture("4.png");
    ret->grid_tex[1] = retrieve_texture("4.png");
    ret->cam_x = ret->cam_y = 100.0;

    int i;
    for (i = 50; i < 120; ++i)
        sim_grid(ret->simulator, 107, i).tag = (i >= 110 || i % 2 == 0);
    for (i = 0; i < 128; ++i) sim_grid(ret->simulator, i, 127).tag = 1;
    for (i = 0; i < 128; ++i) sim_grid(ret->simulator, 127, i).tag = 1;
    ret->simulator->prot.x = ret->simulator->prot.y = 104;
    ret->simulator->prot.w = ret->simulator->prot.h = 0.6;

    return ret;
}
