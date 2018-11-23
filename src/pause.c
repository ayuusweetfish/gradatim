#include "pause.h"

#include "global.h"
#include "label.h"

static inline void pause_tick(pause_scene *this, double dt)
{
}

static inline void pause_draw(pause_scene *this)
{
    SDL_SetRenderTarget(g_renderer, this->a_tex);
    scene_draw(this->a);
    SDL_SetRenderTarget(g_renderer, NULL);
    SDL_RenderCopy(g_renderer, this->a_tex, NULL, NULL);
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 192);
    SDL_RenderFillRect(g_renderer, NULL);
    scene_draw_children((scene *)this);
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
    ret->_base.children = bekter_create();
    ret->_base.tick = (scene_tick_func)pause_tick;
    ret->_base.draw = (scene_draw_func)pause_draw;
    ret->_base.drop = (scene_drop_func)pause_drop;
    ret->_base.key_handler = (scene_key_func)pause_key_handler;
    ret->a = *a;
    ret->b = b;
    ret->p = a;
    ret->a_tex = SDL_CreateTexture(g_renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIN_W, WIN_H);

    label *header = label_create("KiteOne-Regular.ttf", 60,
        (SDL_Color){255, 255, 255}, WIN_H, "PAUSED");
    bekter_pushback(ret->_base.children, header);
    element_place_anchored((element *)header, WIN_W / 2, WIN_H / 4, 0.5, 0.5);

    return ret;
}
