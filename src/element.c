#include "element.h"
#include "global.h"

#include <SDL_image.h>

void element_place(element *e, int x, int y)
{
    e->dim.x = x;
    e->dim.y = y;
}

void element_place_anchored(element *e, float x, float y, float ax, float ay)
{
    e->dim.x = round(x - e->dim.w * ax);
    e->dim.y = round(y - e->dim.h * ay);
}

static void sprite_tick(sprite *this, double dt)
{
}

static void sprite_draw(sprite *this)
{
    render_texture(this->tex, &this->_base.dim);
}

sprite *sprite_create_empty()
{
    sprite *ret = malloc(sizeof(sprite));
    ret->_base.mouse_in = ret->_base.mouse_down = false;
    ret->_base.tick = (element_tick_func)sprite_tick;
    ret->_base.draw = (element_draw_func)sprite_draw;
    ret->_base.drop = NULL;
    ret->tex = (texture){0};
    return ret;
}

sprite *sprite_create(const char *path)
{
    sprite *ret = (sprite *)sprite_create_empty();
    ret->tex = retrieve_texture(path);
    ret->_base.dim.w = ret->tex.range.w;
    ret->_base.dim.h = ret->tex.range.h;
    return ret;
}
