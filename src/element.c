#include "element.h"
#include "global.h"

#include <SDL_image.h>

#include <stdlib.h>

void element_place(element *e, int x, int y)
{
    e->dim.x = x;
    e->dim.y = y;
}

void element_place_anchored(element *e, float x, float y, float ax, float ay)
{
    e->dim.x = iround(x - e->dim.w * ax);
    e->dim.y = iround(y - e->dim.h * ay);
}

static void sprite_draw(sprite *this)
{
    if (this->alpha == 255)
        render_texture(this->tex, &this->_base.dim);
    else
        render_texture_alpha(this->tex, &this->_base.dim, this->alpha);
}

sprite *sprite_create_empty()
{
    sprite *ret = malloc(sizeof(sprite));
    ret->_base.mouse_in = ret->_base.mouse_down = false;
    ret->_base.tick = NULL;
    ret->_base.draw = (element_draw_func)sprite_draw;
    ret->_base.drop = NULL;
    ret->tex = (texture){0};
    ret->alpha = 255;
    return ret;
}

sprite *sprite_create(const char *path)
{
    sprite *ret = (sprite *)sprite_create_empty();
    ret->tex = retrieve_texture(path);
    ret->_base.dim.w = ret->tex.range.w;
    ret->_base.dim.h = ret->tex.range.h;
    ret->alpha = 255;
    return ret;
}

void sprite_reload(sprite *this, const char *path)
{
    this->tex = retrieve_texture(path);
    this->_base.dim.w = this->tex.range.w;
    this->_base.dim.h = this->tex.range.h;
}
