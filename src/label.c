#include "label.h"
#include "global.h"

static unsigned long calc_hash(const char *s)
{
    unsigned long ret = 0;
    int i;
    for (i = 0; s[i] != '\0'; ++i)
        ret = ((ret << 8) ^ (ret >> (s[i] & 3))) + ((i + 1) * s[i]);
    return ret;
}

static void label_render_text(label *this)
{
    /* The exact same text needn't be updated */
    unsigned long hash = calc_hash(this->text);
    if (hash == this->last_hash) return;
    this->last_hash = hash;

    if (this->_base.tex.sdl_tex != NULL)
        SDL_DestroyTexture(this->_base.tex.sdl_tex);

    SDL_Surface *sf = TTF_RenderText_Blended_Wrapped(
        this->font, this->text, this->cl, this->wid
    );
    this->_base.tex =
        temp_texture(SDL_CreateTextureFromSurface(g_renderer, sf));
    SDL_FreeSurface(sf);
    this->_base._base.dim.w = this->_base.tex.range.w;
    this->_base._base.dim.h = this->_base.tex.range.h;
}

static void label_drop(label *this)
{
    SDL_DestroyTexture(this->_base.tex.sdl_tex);
    TTF_CloseFont(this->font);
}

label *label_create(const char *path, int pts,
    SDL_Color cl, int wid, const char *text)
{
    label *ret = (label *)sprite_create_empty();
    ret = realloc(ret, sizeof(label));
    ret->_base._base.drop = (element_drop_func)label_drop;
    ret->font = load_font(path, pts);
    ret->cl = cl;
    ret->text = text;
    ret->wid = wid;
    ret->last_hash = 0;
    label_render_text(ret);
    return ret;
}

void label_set_text(label *this, const char *text)
{
    this->text = text;
    label_render_text(this);
}
