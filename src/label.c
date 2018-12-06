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

static inline int get_arrow_dir(const char ch)
{
    return ch == '^' ? 0 : ch == 'v' ? 1 :
        ch == '<' ? 2 : ch == '>' ? 3 : ch == '\\' ? 4 : -1;
}

static inline double clamp(double x, double l, double u)
{
    return (x < l ? l : (x > u ? u : x));
}

static inline double aa_rect(double x, double y,
    double x1, double x2, double y1, double y2)
{
    double dx = fabs(x - (x1 + x2) / 2);
    double dy = fabs(y - (y1 + y2) / 2);
    dx = clamp((x2 - x1) / 2 - dx, 0, 1);
    dy = clamp((y2 - y1) / 2 - dy, 0, 1);
    return dx < dy ? dx : dy;
}

static inline double blend_line(double ret, double x, double y,
    double w, double x0, double y0, double rot, double len)
{
    /* Calculate point to segment distance */
    /* Note: some modifications are introduced
     * as the edges should not be rounded here */
    /* Rotate the point around (x0, y0) by `rot` radians */
    double _x = (x - x0) * cos(rot) - (y - y0) * sin(rot) + x0;
    double _y = (x - x0) * sin(rot) + (y - y0) * cos(rot) + y0;
    return 1 - (1 - ret) * (1 - aa_rect(_x, _y, x0 - w / 2, x0 + w / 2, y0, y0 + len));
}

/* Generates an upwards arrow */
static inline int uparrow_pixel(int sz, double w, int _x, int _y)
{
    double x = _x + 0.5, y = _y + 0.5;
    double ret = 0;
    ret = blend_line(ret, x, y, w, sz * 0.5, sz * 0.191, 0, sz * 0.618);
    ret = blend_line(ret, x, y, w, sz * 0.5, sz * 0.191, -M_PI * 0.22, sz * 0.25);
    ret = blend_line(ret, x, y, w, sz * 0.5, sz * 0.191, +M_PI * 0.22, sz * 0.25);
    return round(ret * 255);
}

/* Generates an icon for the enter key */
static inline int return_pixel(int sz, double w, int _x, int _y)
{
    double x = _x + 0.5, y = _y + 0.5;
    double ret = 0;
    ret = blend_line(ret, x, y, w, sz * 0.72, sz * 0.25, 0, sz * 0.382);
    ret = blend_line(ret, x, y, w, sz * 0.72, sz * 0.632, -M_PI / 2, sz * 0.5);
    ret = blend_line(ret, x, y, w, sz * 0.22, sz * 0.632, M_PI * 0.69, sz * 0.1545);
    ret = blend_line(ret, x, y, w, sz * 0.22, sz * 0.632, M_PI * 0.31, sz * 0.1545);
    return round(ret * 255);
}

static inline int arrow_pixel_opacity(int dir, int sz, double w, int x, int y)
{
    switch (dir) {
        case 0: return uparrow_pixel(sz, w, x, y);
        case 1: return uparrow_pixel(sz, w, x, sz - 1 - y);
        case 2: return uparrow_pixel(sz, w, y, x);
        case 3: return uparrow_pixel(sz, w, y, sz - 1 - x);
        case 4: return return_pixel(sz, w, x, y);
        default: return 0;
    }
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
        SDL_Surface *tsf = NULL;
        int arrow_dir;
        if ((arrow_dir = get_arrow_dir(keys[n])) != -1) {
            int w = STROKE_W(h);
            tsf = SDL_CreateRGBSurfaceWithFormat(
                0, h, h, 32, SDL_PIXELFORMAT_ARGB8888);
            int i, j;
            for (i = 0; i < h; ++i)
                for (j = 0; j < h; ++j)
                    *((Uint32 *)(tsf->pixels + i * tsf->pitch) + j) =
                        arrow_pixel_opacity(arrow_dir, h - w * 2, 2.5, j - w, i - w) << 24;
        } else if (keys[n] == '~') {
            tsf = TTF_RenderText_Blended(
                load_font(FONT_UPRIGHT, KEY_PTS(h) * 0.6), "ESC", (SDL_Color){0});
        } else {
            tsf = TTF_RenderGlyph_Blended(
                load_font(FONT_UPRIGHT, KEY_PTS(h)), keys[n], (SDL_Color){0});
        }
        /* Draw a circle; `h` is the height and the radius */
        int x0 = (h - tsf->w - 1) / 2 + (arrow_dir == -1 ? OFFS_X(h) : 0),
            y0 = (h - tsf->h - 1) / 2 + (arrow_dir == -1 ? OFFS_Y(h) : 0),
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
