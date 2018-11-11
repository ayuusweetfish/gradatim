#include "transition.h"

void transition_tick(transition_scene *this, double dt)
{
    this->elapsed += dt;
    if (this->elapsed >= this->duration) {
        *(this->p) = this->b;
        SDL_DestroyRenderer(this->a->renderer);
        SDL_DestroyRenderer(this->b->renderer);
        this->a->renderer = this->a_rdr;
        this->a->renderer = this->b_rdr;
    }
}

scene *transition_slidedown_create(
    SDL_Window *win, SDL_Renderer *rdr, scene **a, scene *b, double dur)
{
    transition_scene *ret = malloc(sizeof(transition_scene));
    ret->_base.renderer = rdr;
    ret->_base.tick = (scene_tick_func)transition_tick;
    ret->_base.draw = (scene_draw_func)transition_slidedown_draw;
    ret->a = *a;
    ret->b = b;
    ret->p = a;
    ret->duration = dur;
    ret->elapsed = 0;

    SDL_Renderer *a_rdr = (*a)->renderer, *b_rdr = b->renderer;
    SDL_Renderer
        *a_nrdr = SDL_CreateRenderer(win, -1, SDL_RENDERER_TARGETTEXTURE),
        *b_nrdr = SDL_CreateRenderer(win, -1, SDL_RENDERER_TARGETTEXTURE);
    int w = 800, h = 600;
    SDL_Texture
        *a_tex = SDL_CreateTexture(a_nrdr,
            SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, w, h),
        *b_tex = SDL_CreateTexture(a_nrdr,
            SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, w, h);
    SDL_SetRenderTarget(a_nrdr, a_tex);
    SDL_SetRenderTarget(b_nrdr, b_tex);

    (*a)->renderer = a_nrdr;
    b->renderer = b_nrdr;
    ret->a_rdr = a_rdr;
    ret->b_rdr = b_rdr;
    ret->a_tex = a_tex;
    ret->b_tex = b_tex;

    return (scene *)ret;
}

void transition_slidedown_draw(transition_scene *this)
{
    scene_draw(this->elapsed < this->duration / 2 ? this->b : this->a);
    SDL_RenderCopy(this->_base.renderer,
        this->elapsed < this->duration / 2 ? this->b_tex : this->a_tex,
        NULL, NULL);
}

