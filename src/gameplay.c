#include "gameplay.h"

static void gameplay_scene_tick(gameplay_scene *this, double dt)
{
    sim_tick(this->simulator);
}

static void gameplay_scene_draw(gameplay_scene *this)
{
}

static void gameplay_scene_drop(gameplay_scene *this)
{
    sim_drop(this->simulator);
}

static void gameplay_scene_key_handler(gameplay_scene *this, SDL_KeyboardEvent *ev)
{
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
    return ret;
}
