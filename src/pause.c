#include "pause.h"
#include "global.h"

static inline void pause_tick(pause_scene *this, double dt)
{
}

static inline void pause_draw(pause_scene *this)
{
    SDL_SetRenderTarget(g_renderer, this->a_tex);
    scene_draw(this->a);
    SDL_SetRenderTarget(g_renderer, NULL);
    SDL_RenderCopy(g_renderer, this->a_tex, NULL, NULL);
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 128);
    SDL_RenderFillRect(g_renderer, NULL);
}

static inline void pause_drop(pause_scene *this)
{
    SDL_DestroyTexture(this->a_tex);
}

static inline void pause_key_handler(pause_scene *this, SDL_KeyboardEvent *ev)
{
    if (ev->state != SDL_PRESSED) return;
    switch (ev->keysym.sym) {
        case SDLK_ESCAPE:
            *(this->p) = this->a;
            scene_drop(this);
            break;
        case SDLK_RETURN:
            *(this->p) = this->b;
            scene_drop(this);
            scene_drop(this->a);
            break;
    }
}

pause_scene *pause_scene_create(scene **a, scene *b)
{
    pause_scene *ret = malloc(sizeof(pause_scene));
    ret->_base.children = NULL;
    ret->_base.tick = (scene_tick_func)pause_tick;
    ret->_base.draw = (scene_draw_func)pause_draw;
    ret->_base.drop = (scene_drop_func)pause_drop;
    ret->_base.key_handler = (scene_key_func)pause_key_handler;
    ret->a = *a;
    ret->b = b;
    ret->p = a;
    ret->a_tex = SDL_CreateTexture(g_renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIN_W, WIN_H);
    return ret;
}
