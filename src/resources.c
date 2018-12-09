#include "resources.h"
#include "global.h"

#include <SDL_image.h>
#include <SDL_ttf.h>

#include <string.h>

#define RES_HASH_SZ 997
#define GRID_SZ 256

static bekter(SDL_Texture *) sdl_tex_list;
static bekter(_texture_kvpair) res_map[RES_HASH_SZ];
static texture grid[GRID_SZ];
static int grid_tx[GRID_SZ], grid_ty[GRID_SZ];

static unsigned int elf_hash(const char *s)
{
    unsigned int h = 0, x = 0;
    while (*s != '\0') {
        h = (h << 4) | (*s++);
        if ((x = (h & 0xf0000000L)) != 0) {
            h ^= (x >> 24);
            h &= ~x;
        }
    }
    return (h & 0x7fffffff);
}

static SDL_Texture *texture_from_file(const char *path, int *w, int *h)
{
    SDL_Surface *sfc = IMG_Load(path);
    if (sfc == NULL) return NULL;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(g_renderer, sfc);
    SDL_FreeSurface(sfc);
    SDL_QueryTexture(tex, NULL, NULL, w, h);
    bekter_pushback(sdl_tex_list, tex);
    return tex;
}

static void load_image(const char *path)
{
    /* TODO: Add sprite sheet support */
    int w, h;
    SDL_Texture *tex = texture_from_file(path, &w, &h);
    if (tex == NULL) return;

    char *name = strdup(path);
    if (name == NULL) return;

    unsigned int hash = elf_hash(name) % RES_HASH_SZ;
    _texture_kvpair p = {name, (texture){tex, (SDL_Rect){0, 0, w, h}}};
    bekter_pushback(res_map[hash], p);
}

static void load_grid(const char *image, const char *csv)
{
    SDL_Texture *tex = texture_from_file(image, NULL, NULL);
    if (tex == NULL) return;

    FILE *f = fopen(csv, "r");
    if (f == NULL) { SDL_DestroyTexture(tex); return; }

    int i;
    for (i = 1; i < GRID_SZ; ++i) {
        int x, y, w, h, tx, ty;
        fscanf(f, "%d,%d,%d,%d,%d,%d",
            &x, &y, &w, &h, &tx, &ty);
        grid[i] = (texture){tex, (SDL_Rect){x, y, w, h}};
        grid_tx[i] = tx;
        grid_ty[i] = ty;
    }
    fclose(f);
}

void load_images()
{
    int i;
    sdl_tex_list = bekter_create();
    for (i = 0; i < RES_HASH_SZ; ++i)
        res_map[i] = bekter_create();
    load_image("1.png");
    load_image("2.png");
    load_image("3.png");
    load_image("4.png");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    load_image("qwq.png");
    load_image("uwu.png");
    load_image("block.png");
    load_image("fragile1.png");
    load_image("fragile2.png");
    load_image("fragile3.png");
    load_image("fragile4.png");
    load_image("cloud.png");
    load_image("spring1.png");
    load_image("spring2.png");
    load_image("mushroom_t.png");
    load_image("mushroom_b.png");
    load_image("mushroom_tl.png");
    load_image("refill1.png");
    load_image("refill2.png");
    load_image("refill3.png");
    load_image("refill4.png");
    load_image("puff_l.png");
    load_image("puff_r.png");
    load_image("rua1.png");
    load_image("rua2.png");
    load_grid("grid.png", "grid.csv");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    load_image("clock.png");
    load_image("retry_count.png");
    load_image("minim.png");
    load_image("crotchet.png");
    load_image("quaver.png");
    load_image("cantabile.png");
    load_image("sotto_voco.png");
    load_image("a_capella.png");
    load_image("aria.png");
    load_image("semplice.png");
    load_image("stretto.png");
    load_image("giusto.png");
    load_image("rubato.png");
    load_image("a_piacere.png");
    load_image("hop_4beat.png");
    load_image("dash_4beat.png");
    load_image("hopdash_4beat.png");
}

void release_images()
{
    int i, j;
    SDL_Texture *p;
    for bekter_each(sdl_tex_list, i, p)
        SDL_DestroyTexture(p);
    bekter_drop(sdl_tex_list);
    for (i = 0; i < RES_HASH_SZ; ++i)
        bekter_drop(res_map[i]);
}

static void process_size(texture *t)
{
    int w = 0, h = 0;
    if (t->sdl_tex != NULL)
        SDL_QueryTexture(t->sdl_tex, NULL, NULL, &w, &h);
    t->range = (SDL_Rect){0, 0, w, h};
}

texture retrieve_texture(const char *name)
{
    unsigned int hash = elf_hash(name) % RES_HASH_SZ;
    int i;
    _texture_kvpair p;
    for bekter_each(res_map[hash], i, p) {
        if (strcmp(name, p.name) == 0) return p.value;
    }
    return (texture){0};
}

texture grid_texture(unsigned char idx)
{
    return grid[idx];
}

void grid_offset(unsigned char idx, int *x, int *y)
{
    *x = grid_tx[idx];
    *y = grid_ty[idx];
}

texture temp_texture(SDL_Texture *sdl_tex)
{
    texture ret;
    ret.sdl_tex = sdl_tex;
    process_size(&ret);
    return ret;
}

void render_texture(texture t, SDL_Rect *dim)
{
    if (t.sdl_tex == NULL) return;
    SDL_RenderCopy(g_renderer, t.sdl_tex, &t.range, dim);
}

void render_texture_ex(texture t, SDL_Rect *dim,
    double a, SDL_Point *c, SDL_RendererFlip f)
{
    if (t.sdl_tex == NULL) return;
    SDL_RenderCopyEx(g_renderer, t.sdl_tex, &t.range, dim, a, c, f);
}

void render_texture_scaled(texture t, double x, double y, double scale)
{
    if (t.sdl_tex == NULL) return;
    SDL_RenderCopy(g_renderer, t.sdl_tex, &t.range, &(SDL_Rect){
        x, y, iround(t.range.w * scale), iround(t.range.h * scale)
    });
}

void render_texture_alpha(texture t, SDL_Rect *dim, int alpha)
{
    SDL_SetTextureAlphaMod(t.sdl_tex, alpha);
    render_texture(t, dim);
    SDL_SetTextureAlphaMod(t.sdl_tex, 255);
}

#define N_FONTS 3
#define MAX_PTS 256

/* Build a separate font cache for outlined typefaces,
 * in order to minimize cache flushes done by SDL_ttf.
 * See https://www.libsdl.org/projects/SDL_ttf/docs/SDL_ttf.html#SEC24 */
static const char *FONT_PATH[N_FONTS] = {
    "KiteOne-Regular.ttf",
    "TakoZero-Irregular.ttf",
    "TakoZero-Irregular.ttf"
};
static TTF_Font *f[N_FONTS][MAX_PTS] = {{ NULL }};

TTF_Font *load_font(int id, int pts)
{
    if (id < 0 || id >= N_FONTS || pts <= 0 || pts > MAX_PTS)
        return NULL;

    if (f[id][pts - 1] == NULL) {
        f[id][pts - 1] = TTF_OpenFont(FONT_PATH[id], pts);
        if (id >= FONT_OUTLINE_START)
            TTF_SetFontOutline(f[id][pts - 1], 1);
    }
    return f[id][pts - 1];
}
