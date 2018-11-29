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
    for (i = 0; i < ch->n_stages; ++i) {
        int c = (i == this->cur_stage_idx) ? 255 : 128;
        SDL_SetTextureColorMod(tex[i], c, c, c);
        SDL_RenderCopy(g_renderer, tex[i], NULL, &(SDL_Rect){
            (ch->stages[i]->world_c + ch->stages[i]->cam_c1) * 2,
            (ch->stages[i]->world_r + ch->stages[i]->cam_r1) * 2,
            (ch->stages[i]->cam_c2 - ch->stages[i]->cam_c1) * 2,
            (ch->stages[i]->cam_r2 - ch->stages[i]->cam_r1) * 2
        });
    }
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

#define SAMPLER_TEX_SZ 16
#define PITCH (SAMPLER_TEX_SZ * 4)

static inline Uint32 weighted_average(Uint32 *buf, int r0, int c0)
{
    int i1, j1, i2, j2;
    Uint64 raw_a = 0, ct = 0, rs = 0, gs = 0, bs = 0;
    for (i1 = r0; i1 < r0 + SAMPLER_TEX_SZ / 2; ++i1)
    for (j1 = c0; j1 < c0 + SAMPLER_TEX_SZ / 2; ++j1) {
        int r = (buf[i1 * SAMPLER_TEX_SZ + j1] >> 24),
            g = (buf[i1 * SAMPLER_TEX_SZ + j1] >> 16) & 0xff,
            b = (buf[i1 * SAMPLER_TEX_SZ + j1] >> 8) & 0xff,
            a = (buf[i1 * SAMPLER_TEX_SZ + j1]) & 0xff;
        if (a == 0) continue;
        raw_a += a;
        for (i2 = r0; i2 < r0 + SAMPLER_TEX_SZ / 2; ++i2)
        for (j2 = c0; j2 < c0 + SAMPLER_TEX_SZ / 2; ++j2)
            if (buf[i1 * SAMPLER_TEX_SZ + j1] == buf[i2 * SAMPLER_TEX_SZ + j2]) {
                ct += a;
                rs += r * a;
                gs += g * a;
                bs += b * a;
            }
    }
    if (raw_a == 0) return 0;
    return (round((double)rs / ct) << 24) |
        (round((double)gs / ct) << 16) |
        (round((double)bs / ct) << 8) |
        (round((double)raw_a / (SAMPLER_TEX_SZ * SAMPLER_TEX_SZ / 4)));
}

static inline void retrieve_colour(Uint32 *buf, const texture tex, Uint32 out[4])
{
    if (tex.sdl_tex == NULL) return;
    SDL_RenderClear(g_renderer);
    render_texture(tex, NULL);
    SDL_RenderReadPixels(g_renderer, NULL, SDL_PIXELFORMAT_RGBA8888, buf, PITCH);
    out[0] = weighted_average(buf, 0, 0);
    out[1] = weighted_average(buf, 0, SAMPLER_TEX_SZ / 2);
    out[2] = weighted_average(buf, SAMPLER_TEX_SZ / 2, 0);
    out[3] = weighted_average(buf, SAMPLER_TEX_SZ / 2, SAMPLER_TEX_SZ / 2);
}

static inline Uint32 manipulate(Uint32 colour, int r, int c)
{
    if ((colour & 0xff) == 0) return colour;
    int a = (colour & 0xff) + 3 - ((r * (r + 1) + c * (c + r * 3)) & 7);
    if (a < 0) a = 0; else if (a > 255) a = 255;
    return (colour & 0xffffff00) | a;
}

static inline void load_chapter(overworld_scene *this, const char *path)
{
    struct chap_rec *ch = chap_read(path);
    if (ch == NULL) return;
    bekter_pushback(this->chaps, ch);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_Texture **tex_arr = malloc(sizeof(SDL_Texture *) * ch->n_stages);

    int i, r, c;
    Uint32 repr_colour[256][4] = {{ 0 }};
    SDL_Texture *tmp_tex = SDL_CreateTexture(
        g_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
        SAMPLER_TEX_SZ, SAMPLER_TEX_SZ);
    SDL_SetRenderTarget(g_renderer, tmp_tex);
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 0);
    Uint32 *buf = malloc(SAMPLER_TEX_SZ * PITCH);
    for (i = 0; i < 256; ++i)
        retrieve_colour(buf, ch->stages[0]->grid_tex[i], repr_colour[i]);
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
                Uint32 *cell = repr_colour[
                    st->grid[(r + st->cam_r1) * st->n_cols + (c + st->cam_c1)]
                ];
                pix[(r * 2) * w + (c * 2)] = manipulate(cell[0],
                    r + st->world_r + st->cam_r1, c + st->world_c + st->cam_c1);
                pix[(r * 2) * w + (c * 2 + 1)] = manipulate(cell[1],
                    r + st->world_r + st->cam_r1, c + st->world_c + st->cam_c1 + 256);
                pix[(r * 2 + 1) * w + (c * 2)] = manipulate(cell[2],
                    r + st->world_r + st->cam_r1 + 256, c + st->world_c + st->cam_c1);
                pix[(r * 2 + 1) * w + (c * 2 + 1)] = manipulate(cell[3],
                    r + st->world_r + st->cam_r1 + 256, c + st->world_c + st->cam_c1 + 256);
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

    ret->cam_r = -1000;
    ret->cam_c = -1000;

    return ret;
}
