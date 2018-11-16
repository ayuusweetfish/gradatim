#include "gameplay.h"

#include "global.h"

#include <math.h>

static const float UNIT_PX = 64;
static const float WIN_W_UNITS = (float)WIN_W / UNIT_PX;
static const float WIN_H_UNITS = (float)WIN_H / UNIT_PX;

static const double BEAT = 60.0 / 132;  /* Temporary */

static inline float clamp(float x, float l, float u)
{
    return (x < l ? l : (x > u ? u : x));
}

static void gameplay_scene_tick(gameplay_scene *this, double dt)
{
    double rt = this->rem_time + dt / BEAT;
    while (rt >= SIM_STEPLEN) {
        sim_tick(this->simulator);
        rt -= SIM_STEPLEN;
    }
    this->rem_time = rt;

    /* Move the camera */
    float dest_x = clamp(this->simulator->prot.x,
        WIN_W_UNITS / 2, this->simulator->gcols - WIN_W_UNITS / 2);
    float dest_y = clamp(this->simulator->prot.y,
        WIN_H_UNITS / 2, this->simulator->grows - WIN_H_UNITS / 2);
    float cam_dx = dest_x - (this->cam_x + WIN_W_UNITS / 2);
    float cam_dy = dest_y - (this->cam_y + WIN_H_UNITS / 2);
    float rate = (dt > 0.1 ? 0.1 : dt) * 10;
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

static void gameplay_scene_key_handler(gameplay_scene *this, SDL_KeyboardEvent *ev)
{
    switch (ev->keysym.sym) {
        case SDLK_c:
            this->simulator->prot.vy -= SIM_GRAVITY;
            break;
        case SDLK_LEFT:
            this->simulator->prot.vx =
                (ev->state == SDL_PRESSED ? -1 : 0);
            break;
        case SDLK_RIGHT:
            this->simulator->prot.vx =
                (ev->state == SDL_PRESSED ? 1 : 0);
            break;
        default: break;
    }
}

gameplay_scene *gameplay_scene_create(scene **bg)
{
    gameplay_scene *ret = malloc(sizeof(gameplay_scene));
    ret->_base.children = bekter_create();
    ret->_base.tick = (scene_tick_func)gameplay_scene_tick;
    ret->_base.draw = (scene_draw_func)gameplay_scene_draw;
    ret->_base.drop = (scene_drop_func)gameplay_scene_drop;
    ret->_base.key_handler = (scene_key_func)gameplay_scene_key_handler;
    ret->bg = *bg;
    ret->bg_ptr = bg;

    ret->simulator = sim_create(128, 128);
    ret->rem_time = 0;
    ret->prot_tex = retrieve_texture("1.png");
    ret->grid_tex[1] = retrieve_texture("4.png");
    ret->cam_x = ret->cam_y = 100.0;

    int i;
    for (i = 50; i < 120; ++i)
        sim_grid(ret->simulator, 107, i).tag = (i != 110);
    for (i = 0; i < 128; ++i) sim_grid(ret->simulator, i, 127).tag = 1;
    for (i = 0; i < 128; ++i) sim_grid(ret->simulator, 127, i).tag = 1;
    ret->simulator->prot.x = ret->simulator->prot.y = 104;
    ret->simulator->prot.w = ret->simulator->prot.h = 0.9;

    return ret;
}
