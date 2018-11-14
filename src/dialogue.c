#include "dialogue.h"
#include "global.h"

static void dialogue_tick(dialogue_scene *this, double dt)
{
    scene_tick(this->bg, dt);
    if (this->script_idx >= this->script_len) {
        *(this->bg_ptr) = this->bg;
        scene_drop(this);
    }
}

static void update_children(dialogue_scene *this)
{
    dialogue_entry entry =
        bekter_at(this->script, this->script_idx, dialogue_entry);

    this->avatar_disp->tex = entry.avatar;
    this->avatar_disp->_base.dim.w = 192;
    this->avatar_disp->_base.dim.h = 192;
    element_place_anchored((element *)this->avatar_disp,
        WIN_W / 8, WIN_H * 50 / 72, 0.5, 0.5);

    label_set_text(this->name_disp, entry.name);
    element_place_anchored((element *)this->name_disp,
        WIN_W / 8, WIN_H * 60 / 72, 0.5, 0);

    label_set_text(this->text_disp, entry.text);
    element_place_anchored((element *)this->text_disp,
        WIN_W / 4, WIN_H * 50 / 72, 0, 0);
}

static void dialogue_draw(dialogue_scene *this)
{
    /* Underlying scene */
    SDL_SetRenderTarget(g_renderer, this->bg_tex);
    scene_draw(this->bg);
    SDL_SetRenderTarget(g_renderer, NULL);
    SDL_RenderCopy(g_renderer, this->bg_tex, NULL, NULL);

    /* Text background */
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 192);
    SDL_RenderFillRect(g_renderer, &(SDL_Rect){
        0, WIN_H * 2 / 3, WIN_W, WIN_H / 3
    });

    /* All children; they should have been properly placed beforehand */
    scene_draw_children((scene *)this);
}

static void dialogue_drop(dialogue_scene *this)
{
    bekter_drop(this->script);
    SDL_DestroyTexture(this->bg_tex);
}

static void dialogue_key_handler(dialogue_scene *this, SDL_KeyboardEvent *ev)
{
    if (ev->keysym.sym == SDLK_RETURN) {
        if (++this->script_idx < this->script_len)
            update_children(this);
    }
}

dialogue_scene *dialogue_create(scene **bg, bekter(dialogue_entry) script)
{
    dialogue_scene *ret = malloc(sizeof(dialogue_scene));
    ret->_base.children = bekter_create();
    ret->_base.tick = (scene_tick_func)dialogue_tick;
    ret->_base.draw = (scene_draw_func)dialogue_draw;
    ret->_base.drop = (scene_drop_func)dialogue_drop;
    ret->_base.key_handler = (scene_key_func)dialogue_key_handler;
    ret->bg = *bg;
    ret->bg_ptr = bg;
    ret->script = script;
    ret->script_idx = 0;
    ret->script_len = bekter_size(script) / sizeof(dialogue_entry);

    ret->bg_tex = SDL_CreateTexture(g_renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIN_W, WIN_H);

    /* Avatar sprite */
    sprite *s = sprite_create_empty();
    ret->avatar_disp = s;
    bekter_pushback(ret->_base.children, s);

    /* Name label */
    label *l = label_create("KiteOne-Regular.ttf", 32,
        (SDL_Color){255, 255, 255}, WIN_W / 4, "");
    ret->name_disp = l;
    bekter_pushback(ret->_base.children, l);

    /* Text label */
    l = label_create("KiteOne-Regular.ttf", 32,
        (SDL_Color){255, 255, 255}, WIN_W * 50 / 72, "");
    ret->text_disp = l;
    bekter_pushback(ret->_base.children, l);

    update_children(ret);

    return ret;
}
