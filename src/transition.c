#include "transition.h"

void transition_tick(transition_scene *this, double dt)
{
    this->elapsed += dt;
    if (this->elapsed >= this->duration) {
        *(this->p) = this->b;
    }
}

scene *transition_slidedown_create(
    SDL_Renderer *rdr, scene **a, scene *b, double dur)
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
    return (scene *)ret;
}

void transition_slidedown_draw(transition_scene *this)
{
    scene_draw(this->elapsed < this->duration / 2 ? this->b : this->a);
}

