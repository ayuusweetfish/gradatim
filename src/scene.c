#include "scene.h"
#include "global.h"
#include "overworld.h"
#include "dialogue.h"
#include "transition.h"
#include "element.h"
#include "button.h"
#include "label.h"

void scene_draw_children(scene *this)
{
    int i;
    element *e;
    for bekter_each(this->children, i, e) element_draw(e);
}

static SDL_Point mouse_event_coordinate(SDL_Event *ev, int x, int y)
{
    SDL_Window *window =
        SDL_GetWindowFromID(((SDL_MouseMotionEvent *)ev)->windowID);
    int real_w, real_h;
    SDL_GetWindowSize(window, &real_w, &real_h);
    SDL_Point p;
    p.x = round((double)x / real_w * WIN_W);
    p.y = round((double)y / real_h * WIN_H);
    return p;
}

void scene_handle_mousemove(scene *this, SDL_MouseMotionEvent *ev)
{
    if (this->children == NULL) return;
    SDL_Point p = mouse_event_coordinate((SDL_Event *)ev, ev->x, ev->y);
    int i; element *e;
    for bekter_each(this->children, i, e)
        e->mouse_in = SDL_PointInRect(&p, &e->dim);
}

void scene_handle_mousebutton(scene *this, SDL_MouseButtonEvent *ev)
{
    if (this->children == NULL) return;
    SDL_Point p = mouse_event_coordinate((SDL_Event *)ev, ev->x, ev->y);
    int i; element *e;
    if (ev->state == SDL_PRESSED) {
        for bekter_each(this->children, i, e)
            if (SDL_PointInRect(&p, &e->dim))
                e->mouse_down = true;
    } else {
        for bekter_each(this->children, i, e)
            e->mouse_down = false;
    }
}

void scene_tick_children(scene *this, double dt)
{
    if (this->children == NULL) return;
    int i; element *e;
    for bekter_each(this->children, i, e) if (e->tick) e->tick(e, dt);
}

void scene_clear_children(scene *this)
{
    if (this->children == NULL) return;
    int i;
    element *e;
    for bekter_each(this->children, i, e) element_drop(e);
    bekter_drop(this->children);
}

static void colour_scene_tick(colour_scene *this, double dt)
{
    floue_tick(this->f, dt);
}

static void colour_scene_draw(colour_scene *this)
{
    if ((scene *)this == g_stage) orion_resume(&g_orion, TRACKID_MAIN_BGM);
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_renderer);
    SDL_SetRenderDrawColor(g_renderer, this->r, this->g, this->b, 128);
    SDL_RenderFillRect(g_renderer, NULL);
    SDL_RenderFillRect(g_renderer,
        &(SDL_Rect){WIN_W / 8, WIN_H / 8, WIN_W * 3 / 4, WIN_H * 3 / 4});
    SDL_SetRenderDrawColor(g_renderer, this->r, this->g, this->b, 255);
    SDL_RenderFillRect(g_renderer,
        &(SDL_Rect){WIN_W / 4, WIN_H / 4, WIN_W / 2, WIN_H / 2});
    scene_draw_children((scene *)this);
    floue_draw(this->f);
}

static void colour_scene_key_handler(colour_scene *this, SDL_KeyboardEvent *ev)
{
    if (ev->keysym.sym == SDLK_n) {
        g_stage = transition_slidedown_create(&g_stage,
            (scene *)colour_scene_create(this->b, this->r, this->g), 0.5);
    }
}

static void cb(void *ud)
{
    colour_scene *this = (colour_scene *)ud;
    if ((scene *)this != g_stage) return;

    if (this->r == 255) {
        g_stage = (scene *)overworld_create(g_stage);
        orion_pause(&g_orion, TRACKID_MAIN_BGM);
        return;
    }

    bekter(dialogue_entry) script = bekter_create();
    bekter_pushback(script, ((dialogue_entry){
        retrieve_texture("1.png"), "RAM 1",
        "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
    }));
    bekter_pushback(script, ((dialogue_entry){
        retrieve_texture("2.png"), "RAM 2",
        "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat."
    }));
    bekter_pushback(script, ((dialogue_entry){
        retrieve_texture("1.png"), "RAM 2",
        "Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur."
    }));
    bekter_pushback(script, ((dialogue_entry){
        retrieve_texture("3.png"), "RAM 3",
        "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."
    }));
    g_stage = (scene *)dialogue_create(&g_stage, script);
    bekter_drop(script);
}

colour_scene *colour_scene_create(int r, int g, int b)
{
    colour_scene *ret = malloc(sizeof(colour_scene));
    ret->_base.children = bekter_create();
    ret->_base.tick = (scene_tick_func)colour_scene_tick;
    ret->_base.draw = (scene_draw_func)colour_scene_draw;
    ret->_base.drop = NULL;
    ret->_base.key_handler = (scene_key_func)colour_scene_key_handler;
    ret->r = r;
    ret->g = g;
    ret->b = b;
    ret->f = floue_create((SDL_Color){192, 255, 192});
    floue_add(ret->f, (SDL_Point){WIN_W / 2, WIN_H / 2},
        (SDL_Color){0, 0, 192}, 401);
    button *s = button_create(cb, ret, "1.png", "2.png", "3.png", 1.05, 0.98);
    element_place_anchored((element *)s, WIN_W / 2, WIN_H / 2, 0.5, 0.5);
    bekter_pushback(ret->_base.children, s);
    label *l = label_create("KiteOne-Regular.ttf", 48,
        (SDL_Color){0, 0, 0}, 480, "The spectacle before us was indeed sublime.");
    element_place_anchored((element *)l, WIN_W / 2, WIN_H * 3 / 4, 0.5, 0.5);
    bekter_pushback(ret->_base.children, l);
    return ret;
}
