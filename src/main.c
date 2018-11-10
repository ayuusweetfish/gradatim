/* gcc main.c orion/orion.c orion/libs_wrapper.o -O2 -F /Library/Frameworks -framework SDL2 -lvorbisfile -lsoundtouch -liir -lportaudio -lc++ */
#include "orion/orion.h"

#include <SDL2/SDL.h>

#include <stdbool.h>

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;

    SDL_Window *window = SDL_CreateWindow("",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (window == NULL) return 1;

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) return 1;
    SDL_RenderSetLogicalSize(renderer, 1080, 720);

    struct orion o = orion_create(44100, 2);
    orion_load_ogg(&o, 0, "sketchch.ogg");
    orion_apply_stretch(&o, 0, 1, +3000);
    orion_play_loop(&o, 1, 0, 0, -1);
    orion_overall_play(&o);

    bool running = true;
    SDL_Event e;

    while (running) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) running = false;
        }
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    orion_drop(&o);
    SDL_Quit();

    return 0;
}
