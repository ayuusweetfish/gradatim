#include "orion/orion.h"
#include "global.h"
#include "element.h"
#include "scene.h"
#include "transition.h"
#include "resources.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <stdbool.h>

static void draw_loop()
{
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_renderer);
    scene_tick(g_stage, 1.0 / 60);
    scene_draw(g_stage);
    SDL_RenderPresent(g_renderer);
}

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;
    int img_init = IMG_INIT_PNG;
    if ((IMG_Init(img_init) & img_init) != img_init) return 1;
    if (TTF_Init() < 0) return 1;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    g_window = SDL_CreateWindow("",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WIN_W, WIN_H, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (g_window == NULL) return 1;

    g_renderer = SDL_CreateRenderer(
        g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (g_renderer == NULL) return 1;
    SDL_RenderSetLogicalSize(g_renderer, WIN_W, WIN_H);
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);

    load_images();

    /*struct orion o = orion_create(44100, 2);
    orion_load_ogg(&o, 0, "sketchch.ogg");
    orion_apply_stretch(&o, 0, 1, +3000);
    orion_play_loop(&o, 1, 0, 0, -1);
    orion_overall_play(&o);*/

    g_stage = (scene *)colour_scene_create(0, 192, 255);

    bool running = true;
    SDL_Event e;

    scene *last_stage;
    SDL_MouseMotionEvent last_mousemove;

    unsigned int fps_last_flush = 0, fps_frame_count = 0;

    while (running) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                scene_handle_key(g_stage, &e.key);
            } else if (e.type == SDL_MOUSEMOTION) {
                scene_handle_mousemove(g_stage, &e.motion);
                last_mousemove = e.motion;
            } else if (e.type == SDL_MOUSEBUTTONDOWN ||
                e.type == SDL_MOUSEBUTTONUP)
            {
                scene_handle_mousebutton(g_stage, &e.button);
            }
        }
        if (last_stage != g_stage) {
            last_stage = g_stage;
            scene_handle_mousemove(g_stage, &last_mousemove);
        }
        draw_loop();
        ++fps_frame_count;
        if (fps_last_flush / 1000 != SDL_GetTicks() / 1000) {
            printf("%d FPS\n", fps_frame_count);
            fps_frame_count = 0;
        }
        fps_last_flush = SDL_GetTicks();
    }

    /*orion_drop(&o);*/
    release_images();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
