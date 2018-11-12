#include "scene.h"
#include "global.h"
#include "element.h"

static void draw_elements(scene *this)
{
    int i;
    element *e;
    for bekter_each(this->children, i, e) element_draw(e);
}

void scene_clear_children(scene *this)
{
    if (this->children == NULL) return;
    int i;
    element *e;
    for bekter_each(this->children, i, e) element_drop(e);
    bekter_drop(this->children);
}

static void colour_scene_tick(colour_scene *this, double dt)
{
}

static void colour_scene_draw(colour_scene *this)
{
    SDL_SetRenderDrawColor(this->_base.renderer, 0, 0, 0, 255);
    SDL_RenderClear(this->_base.renderer);
    SDL_SetRenderDrawColor(this->_base.renderer, this->r, this->g, this->b, 128);
    SDL_RenderFillRect(this->_base.renderer, NULL);
    SDL_RenderFillRect(this->_base.renderer,
        &(SDL_Rect){WIN_W / 8, WIN_H / 8, WIN_W * 3 / 4, WIN_H * 3 / 4});
    SDL_SetRenderDrawColor(this->_base.renderer, this->r, this->g, this->b, 255);
    SDL_RenderFillRect(this->_base.renderer,
        &(SDL_Rect){WIN_W / 4, WIN_H / 4, WIN_W / 2, WIN_H / 2});
    draw_elements((scene *)this);
}

static void colour_scene_drop(colour_scene *this)
{
    free(this);
}

scene *colour_scene_create(SDL_Renderer *rdr, int r, int g, int b)
{
    colour_scene *ret = malloc(sizeof(colour_scene));
    ret->_base.renderer = rdr;
    ret->_base.children = bekter_create();
    ret->_base.tick = (scene_tick_func)colour_scene_tick;
    ret->_base.draw = (scene_draw_func)colour_scene_draw;
    ret->_base.drop = (scene_drop_func)colour_scene_drop;
    ret->r = r;
    ret->g = g;
    ret->b = b;
    sprite *s = sprite_create(rdr, "1.png");
    element_place_anchored(s, WIN_W / 2, WIN_H / 2, 0.5, 0.5);
    bekter_pushback(ret->_base.children, s);
    return (scene *)ret;
}
