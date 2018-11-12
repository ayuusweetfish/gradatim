#include "scene.h"

scene *colour_scene_create(SDL_Renderer *rdr, int r, int g, int b)
{
    colour_scene *ret = malloc(sizeof(colour_scene));
    ret->_base.renderer = rdr;
    ret->_base.tick = (scene_tick_func)colour_scene_tick;
    ret->_base.draw = (scene_draw_func)colour_scene_draw;
    ret->r = r;
    ret->g = g;
    ret->b = b;
    return (scene *)ret;
}

void colour_scene_tick(colour_scene *this, double dt)
{
}

void colour_scene_draw(colour_scene *this)
{
    SDL_SetRenderDrawColor(this->_base.renderer, this->r, this->g, this->b, 255);
    SDL_RenderFillRect(this->_base.renderer, NULL);
}

