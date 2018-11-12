#include "transition.h"
#include "global.h"

#include <math.h>

static void _transition_tick(transition_scene *this, double dt)
{
    this->elapsed += dt;
    if (this->elapsed >= this->duration) {
        *(this->p) = this->b;
        SDL_DestroyTexture(this->a_tex);
        SDL_DestroyTexture(this->b_tex);
    }
}

static void _transition_draw(transition_scene *this)
{
    SDL_SetRenderTarget(this->_base.renderer, this->a_tex);
    scene_draw(this->a);
    SDL_SetRenderTarget(this->_base.renderer, this->b_tex);
    scene_draw(this->b);
    SDL_SetRenderTarget(this->_base.renderer, this->orig_target);
    this->t_draw(this);
}

static transition_scene *_transition_create(scene **a, scene *b, double dur)
{
    SDL_Renderer *rdr = (*a)->renderer;
    if (b->renderer != rdr) return NULL;

    transition_scene *ret = malloc(sizeof(transition_scene));
    ret->_base.renderer = rdr;
    ret->_base.tick = (scene_tick_func)_transition_tick;
    ret->_base.draw = (scene_draw_func)_transition_draw;
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

static void _transition_slidedown_draw(transition_scene *this)
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
    transition_scene *ret = _transition_create(a, b, dur);
    if (ret == NULL) return NULL;
    ret->t_draw = _transition_slidedown_draw;
    return (scene *)ret;
}
