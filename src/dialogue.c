#include "dialogue.h"
#include "global.h"

#include <string.h>

static const double DELAY_PER_CHAR = 0.025;

static void preprocess_text(char *str, TTF_Font *font, int wrap_w)
{
    int len = strlen(str);
    int pos = 0;
    int w;
    while (pos < len) {
        int lo = pos + 1, hi = len + 1, mid;
        while (lo < hi - 1) {
            mid = (lo + hi) >> 1;
            char tmp = str[mid];
            str[mid] = '\0';
            if (TTF_SizeText(font, str + pos, &w, NULL) != 0) return;
            str[mid] = tmp;
            if (w > wrap_w) hi = mid; else lo = mid;
        }
        if (lo == len) break;
        while (!isspace(str[lo]) && lo > pos) --lo;
        if (lo == pos) return;
        str[lo] = '\n';
        pos = lo + 1;
    }
}

static void dialogue_tick(dialogue_scene *this, double dt)
{
    scene_tick(this->bg, dt);
    if (this->script_idx >= this->script_len) {
        *(this->bg_ptr) = this->bg;
        scene_drop(this);
        return;
    }

    dialogue_entry entry =
        bekter_at(this->script, this->script_idx, dialogue_entry);
    this->entry_lasted += dt;
    int textpos = (int)(this->entry_lasted / DELAY_PER_CHAR);
    if (textpos > entry.text_len) textpos = entry.text_len;
    if (this->last_textpos != textpos) {
        this->last_textpos = textpos;
        char t = entry.text[textpos];
        entry.text[textpos] = '\0';
        label_set_text(this->text_disp, entry.text);
        entry.text[textpos] = t;
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

    label_set_text(this->text_disp, "");
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
    if (ev->keysym.sym != SDLK_RETURN) return;
    dialogue_entry entry =
        bekter_at(this->script, this->script_idx, dialogue_entry);
    int textpos = (int)(this->entry_lasted / DELAY_PER_CHAR);
    if (textpos < entry.text_len) {
        this->entry_lasted += entry.text_len * DELAY_PER_CHAR;
    } else if (++this->script_idx < this->script_len) {
        update_children(this);
        this->entry_lasted = 0;
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
    ret->entry_lasted = 0;
    ret->last_textpos = -1;

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

    int i;
    dialogue_entry *entry;
    for bekter_each_ptr(ret->script, i, entry) {
        entry->name = strdup(entry->name);
        entry->text = strdup(entry->text);
        entry->text_len = strlen(entry->text);
        preprocess_text(entry->text, l->font, l->wid);
    }

    update_children(ret);

    return ret;
}
