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
    SDL_RenderCopy(this->_base.renderer, this->tex, NULL, &this->_base.dim);
}

static void sprite_drop(sprite *this)
{
    SDL_DestroyTexture(this->tex);
    free(this);
}

element *sprite_create(SDL_Renderer *rdr, const char *path)
{
    sprite *ret = malloc(sizeof(sprite));
    ret->_base.renderer = rdr;
    ret->_base.tick = (element_tick_func)sprite_tick;
    ret->_base.draw = (element_draw_func)sprite_draw;
    ret->_base.drop = (element_drop_func)sprite_drop;

    SDL_Surface *sfc = IMG_Load(path);
    if (sfc == NULL) return NULL;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(rdr, sfc);
    SDL_FreeSurface(sfc);
    SDL_QueryTexture(tex, NULL, NULL, &ret->_base.dim.w, &ret->_base.dim.h);
    ret->tex = tex;

    return (element *)ret;
}
