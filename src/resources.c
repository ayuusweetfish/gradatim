#include "resources.h"
#include "global.h"

#include <SDL_image.h>

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
    return tex;
}

static void load_image(const char *path)
{
    int w, h;
    SDL_Texture *tex = texture_from_file(path, &w, &h);
    if (tex == NULL) return;

    char *name = strdup(path);
    if (name == NULL) return;

    unsigned int hash = elf_hash(name) % RES_HASH_SZ;
    _texture_kvpair p = {name, tex};
    bekter_pushback(res_map[hash], p);
}

void load_images()
{
    load_image("1.png");
    load_image("2.png");
    load_image("3.png");
}

void finalize_images()
{
}

SDL_Texture *retrieve_sdl_texture(const char *name)
{
    unsigned int hash = elf_hash(name) % RES_HASH_SZ;
    int i;
    _texture_kvpair p;
    for bekter_each(res_map[hash], i, p) {
        if (strcmp(name, p.name) == 0) return p.sdl_tex;
    }
    return NULL;
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
    /* TODO: Add sprite sheet support */
    texture ret;
    ret.sdl_tex = retrieve_sdl_texture(name);
    process_size(&ret);
    return ret;
}

texture temp_texture(const SDL_Texture *sdl_tex)
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
