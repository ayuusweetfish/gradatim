#include "transition.h"
#include "global.h"

#include <math.h>

static void transition_tick(transition_scene *this, double dt)
{
    this->elapsed += dt;
    if (this->elapsed >= this->duration) {
        *(this->p) = this->b;
        scene_drop(this->a);
        scene_drop(this);
    }
}

static void transition_draw(transition_scene *this)
{
    SDL_SetRenderTarget(this->_base.renderer, this->a_tex);
    scene_draw(this->a);
    SDL_SetRenderTarget(this->_base.renderer, this->b_tex);
    scene_draw(this->b);
    SDL_SetRenderTarget(this->_base.renderer, this->orig_target);
    this->t_draw(this);
}

static void transition_drop(transition_scene *this)
{
    SDL_DestroyTexture(this->a_tex);
    SDL_DestroyTexture(this->b_tex);
}

static transition_scene *transition_create(scene **a, scene *b, double dur)
{
    SDL_Renderer *rdr = (*a)->renderer;
    if (b->renderer != rdr) return NULL;

    transition_scene *ret = malloc(sizeof(transition_scene));
    ret->_base.renderer = rdr;
    ret->_base.children = NULL;
    ret->_base.tick = (scene_tick_func)transition_tick;
    ret->_base.draw = (scene_draw_func)transition_draw;
    ret->_base.drop = (scene_drop_func)transition_drop;
    ret->a = *a;
    ret->b = b;
    ret->p = a;
    ret->duration = dur;
    ret->elapsed = 0;
    ret->orig_target = SDL_GetRenderTarget(rdr);

    ret->a_tex = SDL_CreateTexture(rdr,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIN_W, WIN_H),
    ret->b_tex = SDL_CreateTexture(rdr,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIN_W, WIN_H);

    return ret;
}

static void transition_slidedown_draw(transition_scene *this)
{
    double p = this->elapsed / this->duration;
    double q = 0.5 * (1 - cos(p * M_PI));
    SDL_RenderCopy(this->_base.renderer, this->a_tex,
        &((SDL_Rect){0, 0, WIN_W, round((1 - q) * WIN_H)}),
        &((SDL_Rect){0, round(q * WIN_H), WIN_W, round((1 - q) * WIN_H)}));
    SDL_RenderCopy(this->_base.renderer, this->b_tex,
        &((SDL_Rect){0, round((1 - q) * WIN_H), WIN_W, round(q * WIN_H)}),
        &((SDL_Rect){0, 0, WIN_W, round(q * WIN_H)}));
}

scene *transition_slidedown_create(scene **a, scene *b, double dur)
{
    transition_scene *ret = transition_create(a, b, dur);
    if (ret == NULL) return NULL;
    ret->t_draw = transition_slidedown_draw;
    return (scene *)ret;
}
