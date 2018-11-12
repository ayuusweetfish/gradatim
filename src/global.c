#include "global.h"

#include <SDL_image.h>

SDL_Window *g_window;
SDL_Renderer *g_renderer;
scene *g_stage;

SDL_Texture *load_texture(const char *path, int *w, int *h)
{
    SDL_Surface *sfc = IMG_Load(path);
    if (sfc == NULL) return NULL;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(g_renderer, sfc);
    SDL_FreeSurface(sfc);
    SDL_QueryTexture(tex, NULL, NULL, w, h);
    return tex;
}
