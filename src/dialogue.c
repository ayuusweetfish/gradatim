#include "dialogue.h"
#include "global.h"

#include <string.h>

static const double CTNR_FADE_DUR = 0.2;
static const double AVAT_FADE_DUR = 0.1;
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
    this->entry_lasted += dt;

    if (this->script_idx >= this->script_len) {
        if (this->entry_lasted >= AVAT_FADE_DUR + CTNR_FADE_DUR) {
            *(this->bg_ptr) = this->bg;
            scene_drop(this);
        }
        return;
    }

    dialogue_entry entry =
        bekter_at(this->script, this->script_idx, dialogue_entry);
    if (this->entry_lasted >= AVAT_FADE_DUR) {
        int textpos = (this->entry_lasted - AVAT_FADE_DUR * 2) / DELAY_PER_CHAR;
        if (textpos > entry.text_len) textpos = entry.text_len;
        if (textpos < 0) textpos = 0;
        if (this->last_textpos != textpos) {
            this->last_textpos = textpos;
            char t = entry.text[textpos];
            entry.text[textpos] = '\0';
            label_set_text(this->text_disp, entry.text);
            entry.text[textpos] = t;
        }
    }

    /* Update display for animations */
    if (this->last_tick < AVAT_FADE_DUR && this->entry_lasted >= AVAT_FADE_DUR) {
        this->avatar_disp->tex = entry.avatar;
        this->avatar_disp->_base.dim.w = 192;
        this->avatar_disp->_base.dim.h = 192;
        element_place_anchored((element *)this->avatar_disp,
            WIN_W / 8, WIN_H * 50 / 72, 0.5, 0.5);
        if (entry.name != NULL) {
            label_set_text(this->name_disp, entry.name);
            element_place_anchored((element *)this->name_disp,
                WIN_W / 8, WIN_H * 60 / 72, 0.5, 0);
        }
    }
    this->last_tick = this->entry_lasted;
}

static void dialogue_draw(dialogue_scene *this)
{
    /* Underlying scene */
    SDL_SetRenderTarget(g_renderer, this->bg_tex);
    scene_draw(this->bg);
    SDL_SetRenderTarget(g_renderer, NULL);
    SDL_RenderCopy(g_renderer, this->bg_tex, NULL, NULL);

    bool past_the_end = this->script_idx >= this->script_len;
    bool character_same = !past_the_end &&
        bekter_at(this->script, this->script_idx, dialogue_entry).name == NULL;

    /* Text's background */
    if (this->entry_lasted < 0 ||
        (past_the_end && this->entry_lasted >= AVAT_FADE_DUR &&
        this->entry_lasted < AVAT_FADE_DUR + CTNR_FADE_DUR))
    {
        double p;
        if (this->entry_lasted < 0)
            p = 1 + this->entry_lasted / CTNR_FADE_DUR;
        else p = 1 - (this->entry_lasted - AVAT_FADE_DUR) / CTNR_FADE_DUR;
        p = 1 - (1 - p) * (1 - p) * (1 - p);
        int opacity = round(192 * p);
        int height = WIN_H * 3 / 12 + round(WIN_H / 12 * p);
        SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, opacity);
        SDL_RenderFillRect(g_renderer, &(SDL_Rect){
            0, WIN_H - height, WIN_W, height
        });
    } else {
        SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 192);
        SDL_RenderFillRect(g_renderer, &(SDL_Rect){
            0, WIN_H * 2 / 3, WIN_W, WIN_H / 3
        });
    }

    /* Avatar & name */
    int opacity;
    if (this->entry_lasted < AVAT_FADE_DUR)
        opacity = round(255 * (1 - this->entry_lasted / AVAT_FADE_DUR));
    else if (!past_the_end && this->entry_lasted < 2 * AVAT_FADE_DUR)
        opacity = round(255 * (this->entry_lasted / AVAT_FADE_DUR - 1));
    else opacity = past_the_end ? 0 : 255;
    this->avatar_disp->alpha = character_same ? 255 : opacity;
    this->name_disp->_base.alpha = character_same ? 255 : opacity;
    this->text_disp->_base.alpha = opacity;
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
    int textpos = (this->entry_lasted - AVAT_FADE_DUR * 2) / DELAY_PER_CHAR;
    if (textpos < entry.text_len) {
        this->entry_lasted +=
            CTNR_FADE_DUR + AVAT_FADE_DUR * 2 + entry.text_len * DELAY_PER_CHAR;
    } else {
        ++this->script_idx;
        this->last_tick = this->entry_lasted = 0;
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
    ret->last_tick = ret->entry_lasted = -CTNR_FADE_DUR;
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
    element_place((element *)l, WIN_W / 4, WIN_H * 50 / 72);

    int i;
    dialogue_entry *entry;
    char *last_name = NULL;
    for bekter_each_ptr(ret->script, i, entry) {
        if (i != 0 && strcmp(last_name, entry->name) == 0)
            entry->name = NULL;
        else
            last_name = entry->name = strdup(entry->name);
        entry->text = strdup(entry->text);
        entry->text_len = strlen(entry->text);
        preprocess_text(entry->text, l->font, l->wid);
    }

    return ret;
}
