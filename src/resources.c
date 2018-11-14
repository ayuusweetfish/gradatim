#include "resources.h"
#include "global.h"

#include <SDL_image.h>

bekter(SDL_Texture *) sdl_tex_list;
bekter(_texture_kvpair) res_map[RES_HASH_SZ];

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

void load_images()
{
    int i;
    sdl_tex_list = bekter_create();
    for (i = 0; i < RES_HASH_SZ; ++i)
        res_map[i] = bekter_create();
    load_image("1.png");
    load_image("2.png");
    load_image("3.png");
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

void render_texture_alpha(texture t, SDL_Rect *dim, int alpha)
{
    SDL_SetTextureAlphaMod(t.sdl_tex, alpha);
    render_texture(t, dim);
    SDL_SetTextureAlphaMod(t.sdl_tex, 255);
}
