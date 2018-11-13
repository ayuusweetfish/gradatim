#include "label.h"
#include "global.h"

static void label_render_text(label *this)
{
    /* TODO: Do not render if the text has not changed */
    if (this->_base.tex != NULL)
        SDL_DestroyTexture(this->_base.tex);

    SDL_Surface *sf = TTF_RenderText_Blended_Wrapped(
        this->font, this->text, this->cl, this->_base._base.dim.w
    );
    this->_base.tex = SDL_CreateTextureFromSurface(g_renderer, sf);
    SDL_FreeSurface(sf);
    SDL_QueryTexture(this->_base.tex, NULL, NULL, NULL, &this->_base._base.dim.h);
}

element *label_create(const char *path, int pts,
    SDL_Color cl, int wid, const char *text)
{
    label *ret = (label *)sprite_create_empty();
    ret = realloc(ret, sizeof(label));
    ret->font = load_font(path, pts);
    ret->cl = cl;
    ret->_base._base.dim.w = wid;
    ret->text = text;
    label_render_text(ret);
    return (element *)ret;
}
