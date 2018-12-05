#include "label.h"
#include "global.h"
#include "resources.h"

#include <assert.h>

static const int PADDING = 2;
#define STROKE_W(_h) ((_h) / 20)
#define KEY_PTS(_h) ((_h) * 2 / 3)
#define OFFS_X(_h)  ((_h) / 25)
#define OFFS_Y(_h)  ((_h) / 20)

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

static void label_render_keyed_text(label *this, const char *keys)
{
    /* Don't need to use caching for this type */
    if (this->_base.tex.sdl_tex != NULL)
        SDL_DestroyTexture(this->_base.tex.sdl_tex);

    SDL_Surface *sf = TTF_RenderText_Blended_Wrapped(
        this->font, this->text, this->cl, this->wid
    );

    if (SDL_MUSTLOCK(sf)) SDL_LockSurface(sf);
    assert(sf->format->format == SDL_PIXELFORMAT_ARGB8888);

    /* Process text for keys */
    char *s = strdup(this->text);
    char *t = s - 1;
    int n = 0;  /* Number of replacements performed */
    while ((t = strchr(t + 1, '`')) != NULL) {
        char orig_t = *t;
        *t = '\0';
        int x;
        TTF_SizeText(this->font, s, &x, NULL);

        int h = sf->h - PADDING * 2;
        /* Render the key icon */
        SDL_Surface *tsf = TTF_RenderGlyph_Blended(
            load_font(FONT_UPRIGHT, KEY_PTS(h)), keys[n], (SDL_Color){0});
        /* Draw a circle; `h` is the height and the radius */
        int x0 = (h - tsf->w - 1) / 2 + OFFS_X(h),
            y0 = (h - tsf->h - 1) / 2 + OFFS_Y(h),
            w = STROKE_W(h);
        int i, j;
        for (i = -PADDING; i < h + PADDING; ++i)
            for (j = -PADDING; j < h; ++j) {
                /* `j` does not exceed `h`, so that
                 * the succeeding characters won't be erased */
                Uint32 pix = 0x0;   /* Transparent */
                double d = sqrt(sqr(i + 0.5 - h * 0.5) + sqr(j + 0.5 - h * 0.5));
                if (d <= h * 0.5 - 1 - w) {
                    /* White */
                    pix = 0xffffffff;
                    if (i >= y0 && i < y0 + tsf->h && j >= x0 && j < x0 + tsf->w) {
                        Uint32 tsf_pix =
                            *((Uint32 *)(tsf->pixels + (i - y0) * tsf->pitch) + j - x0);
                        int grey = (tsf_pix & (tsf->format->Amask)) >> tsf->format->Ashift;
                        grey = 255 - grey;
                        if (grey != 255)
                            pix = 0xff000000 | (grey << 16) | (grey << 8) | grey;
                    }
                } else if (d <= h * 0.5 - w) {
                    /* White-black gradient */
                    int grey = round((h * 0.5 - w - d) * 255);
                    pix = 0xff000000 | (grey << 16) | (grey << 8) | grey;
                } else if (d <= h * 0.5 - 1) {
                    /* Black */
                    pix = 0xff000000;
                } else if (d <= h * 0.5) {
                    /* Black-transparent gradient */
                    int alpha = round((h * 0.5 - d) * 255);
                    pix = (alpha << 24) | 0x0;
                }
                *((Uint32 *)(sf->pixels + sf->pitch * (i + PADDING)) +
                    (j + x + PADDING - h / 2)) = pix;
            }

        ++n;
        *t = orig_t;
        SDL_FreeSurface(tsf);
    }
    free(s);

    if (SDL_MUSTLOCK(sf)) SDL_UnlockSurface(sf);

    this->_base.tex =
        temp_texture(SDL_CreateTextureFromSurface(g_renderer, sf));
    SDL_FreeSurface(sf);
    this->_base._base.dim.w = this->_base.tex.range.w;
    this->_base._base.dim.h = this->_base.tex.range.h;
}

static void label_drop(label *this)
{
    SDL_DestroyTexture(this->_base.tex.sdl_tex);
}

label *label_create(int font_id, int pts,
    SDL_Color cl, int wid, const char *text)
{
    label *ret = (label *)sprite_create_empty();
    ret = realloc(ret, sizeof(label));
    ret->_base._base.drop = (element_drop_func)label_drop;
    ret->font = load_font(font_id, pts);
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

void label_set_keyed_text(label *this, const char *text, const char *keys)
{
    this->text = text;
    label_render_keyed_text(this, keys);
}

void label_colour_mod(label *this, Uint8 r, Uint8 g, Uint8 b)
{
    SDL_SetTextureColorMod(this->_base.tex.sdl_tex, r, g, b);
}
