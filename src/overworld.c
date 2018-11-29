#include "overworld.h"
#include "global.h"
#include "gameplay.h"

#include <stdlib.h>

static const int PIX_PER_UNIT = 2;

static void ow_tick(overworld_scene *this, double dt)
{
}

static void ow_draw(overworld_scene *this)
{
    SDL_SetRenderDrawColor(g_renderer, 216, 255, 192, 255);
    SDL_RenderClear(g_renderer);

    int i;
    struct chap_rec *ch = bekter_at(this->chaps, this->cur_chap_idx, typeof(ch));
    SDL_Texture **tex = bekter_at(this->stage_tex, this->cur_chap_idx, typeof(tex));
    /*for (i = 0; i < ch->n_stages; ++i) {
        SDL_RenderCopy(g_renderer, tex[i], NULL, NULL);
    }*/
    SDL_RenderCopy(g_renderer, tex[this->cur_stage_idx], NULL, NULL);
}

static void ow_drop(overworld_scene *this)
{
    int i;
    struct chap_rec *p;
    for bekter_each(this->chaps, i, p) chap_drop(p);
    bekter_drop(this->chaps);
    free(this);
}

static void ow_key(overworld_scene *this, SDL_KeyboardEvent *ev)
{
    if (ev->state != SDL_PRESSED) return;
    switch (ev->keysym.sym) {
        case SDLK_ESCAPE:
            g_stage = this->bg;
            ow_drop(this);
            break;
        case SDLK_SPACE:
            g_stage = (scene *)gameplay_scene_create(&g_stage,
                bekter_at(this->chaps, 0, struct chap_rec *), this->cur_stage_idx);
            break;
        case SDLK_LEFT:
            if (this->cur_stage_idx > 0) this->cur_stage_idx--;
            break;
        case SDLK_RIGHT:
            if (this->cur_stage_idx <
                bekter_at(this->chaps, this->cur_chap_idx, struct chap_rec *)->n_stages - 1)
                this->cur_stage_idx++;
            break;
    }
}

#define SAMPLER_TEX_SZ 64

/* TODO: Averaging does not work well, make modifications */
static inline Uint32 retrieve_colour(Uint32 *buf, const texture tex)
{
    if (tex.sdl_tex == NULL) return 0;
    render_texture(tex, NULL);
    SDL_RenderReadPixels(g_renderer, NULL,
        SDL_PIXELFORMAT_RGBA8888, buf, SAMPLER_TEX_SZ * 4);
    int i;
    int ct = 0, r_sum = 0, g_sum = 0, b_sum = 0;
    for (i = 0; i < SAMPLER_TEX_SZ * SAMPLER_TEX_SZ; ++i) {
        Uint32 pix = buf[i];
        ct += pix & 0xff;
        r_sum += (pix & 0xff) * (pix >> 24);
        g_sum += (pix & 0xff) * ((pix >> 16) & 0xff);
        b_sum += (pix & 0xff) * ((pix >> 8) & 0xff);
    }
    return (round((double)r_sum / ct) << 24) |
        (round((double)r_sum / ct) << 16) |
        (round((double)b_sum / ct) << 8) | 0xff;
}

static inline void load_chapter(overworld_scene *this, const char *path)
{
    struct chap_rec *ch = chap_read(path);
    if (ch == NULL) return;
    bekter_pushback(this->chaps, ch);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_Texture **tex_arr = malloc(sizeof(SDL_Texture *) * ch->n_stages);

    int i, r, c;
    Uint32 repr_colour[256];
    SDL_Texture *tmp_tex = SDL_CreateTexture(
        g_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
        SAMPLER_TEX_SZ, SAMPLER_TEX_SZ);
    SDL_SetRenderTarget(g_renderer, tmp_tex);
    Uint32 *buf = malloc(SAMPLER_TEX_SZ * SAMPLER_TEX_SZ * 4);
    for (i = 0; i < 256; ++i)
        repr_colour[i] = retrieve_colour(buf, ch->stages[0]->grid_tex[i]);
    SDL_SetRenderTarget(g_renderer, NULL);
    SDL_DestroyTexture(tmp_tex);
    free(buf);

    for (i = 0; i < ch->n_stages; ++i) {
        struct stage_rec *st = ch->stages[i];
        int nr = st->cam_r2 - st->cam_r1,
            nc = st->cam_c2 - st->cam_c1;
        int h = nr * PIX_PER_UNIT, w = nc * PIX_PER_UNIT;
        SDL_Texture *tex = SDL_CreateTexture(
            g_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, w, h
        );
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        Uint32 *pix = malloc(w * h * 4);
        memset(pix, 0, w * h * 4);
        for (r = 0; r < nr; ++r)
            for (c = 0; c < nc; ++c) {
                Uint32 cell = repr_colour[
                    st->grid[(r + st->cam_r1) * st->n_cols + (c + st->cam_c1)]
                ];
                pix[(r * 2) * w + (c * 2)] =
                pix[(r * 2) * w + (c * 2 + 1)] =
                pix[(r * 2 + 1) * w + (c * 2)] =
                pix[(r * 2 + 1) * w + (c * 2 + 1)] = cell;
            }
        SDL_UpdateTexture(tex, NULL, pix, w * 4);
        free(pix);
        tex_arr[i] = tex;
    }

    bekter_pushback(this->stage_tex, tex_arr);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
}

overworld_scene *overworld_create(scene *bg)
{
    overworld_scene *ret = malloc(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));
    ret->_base.tick = (scene_tick_func)ow_tick;
    ret->_base.draw = (scene_draw_func)ow_draw;
    ret->_base.drop = (scene_drop_func)ow_drop;
    ret->_base.key_handler = (scene_key_func)ow_key;
    ret->bg = bg;

    ret->chaps = bekter_create();
    ret->stage_tex = bekter_create();
    load_chapter(ret, "chap.csv");

    ret->cur_chap_idx = 0;
    ret->cur_stage_idx = 0;

    return ret;
}
