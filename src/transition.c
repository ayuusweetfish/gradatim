#include "transition.h"

void transition_tick(transition_scene *this, double dt)
{
    this->elapsed += dt;
    if (this->elapsed >= this->duration) {
        *(this->p) = this->b;
        SDL_DestroyTexture(this->a_tex);
        SDL_DestroyTexture(this->b_tex);
    }
}

scene *transition_slidedown_create(scene **a, scene *b, double dur)
{
    SDL_Renderer *rdr = (*a)->renderer;
    if (b->renderer != rdr) return NULL;

    transition_scene *ret = malloc(sizeof(transition_scene));
    ret->_base.renderer = rdr;
    ret->_base.tick = (scene_tick_func)transition_tick;
    ret->_base.draw = (scene_draw_func)transition_slidedown_draw;
    ret->a = *a;
    ret->b = b;
    ret->p = a;
    ret->duration = dur;
    ret->elapsed = 0;
    ret->orig_target = SDL_GetRenderTarget(rdr);

    int w = 800, h = 600;
    ret->a_tex = SDL_CreateTexture(rdr,
        SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, w, h),
    ret->b_tex = SDL_CreateTexture(rdr,
        SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, w, h);

    return (scene *)ret;
}

void transition_slidedown_draw(transition_scene *this)
{
    SDL_SetRenderTarget(this->_base.renderer, this->a_tex);
    scene_draw(this->a);
    SDL_SetRenderTarget(this->_base.renderer, this->b_tex);
    scene_draw(this->b);
    SDL_SetRenderTarget(this->_base.renderer, this->orig_target);
    SDL_RenderCopy(this->_base.renderer,
        this->elapsed < this->duration / 2 ? this->b_tex : this->a_tex,
        NULL, NULL);
}

