#include "overworld.h"
#include "global.h"
#include "loading.h"
#include "overworld_menu.h"
#include "gameplay.h"
#include "transition.h"
#include "profile_data.h"

#include <stdlib.h>

#define min(_a, _b) ((_a) < (_b) ? (_a) : (_b))

static const int PIX_PER_UNIT = 2;
static const double CAM_MOV_FAC = 8;
static const double NAV_W = WIN_W * 0.8;
static const double NAV_H = WIN_H * 0.9;
static const double MAX_SCALE = WIN_H / 100;
static const double CHAP_SW_DUR = 0.3;
static const double KEYHINT_DUR = 0.2;

static inline void move_camera(overworld_scene *this)
{
    struct chap_rec *ch = bekter_at(this->chaps, this->cur_chap_idx, struct chap_rec *);
    struct stage_rec *st = ch->stages[this->cur_stage_idx];
    int x = (st->world_c + st->cam_c1) * PIX_PER_UNIT,
        y = (st->world_r + st->cam_r1) * PIX_PER_UNIT,
        w = (st->cam_c2 - st->cam_c1) * PIX_PER_UNIT,
        h = (st->cam_r2 - st->cam_r1) * PIX_PER_UNIT;
    this->cam_targx = x + w / 2;
    this->cam_targy = y + h / 2;
    this->cam_targscale = min(NAV_W / w, NAV_H / h);
    this->cam_targscale = min(MAX_SCALE, this->cam_targscale);
}

static inline void update_camera(overworld_scene *this, double rate)
{
    double cam_dx = this->cam_targx - this->cam_x;
    double cam_dy = this->cam_targy - this->cam_y;
    double cam_ds = this->cam_targscale - this->cam_scale;
    this->cam_x += rate * cam_dx;
    this->cam_y += rate * cam_dy;
    this->cam_scale += rate * cam_ds;
}

/* Returns whether new stages are available _in the current chapter_
 * Note: return value is meaningful only after stage clear */
static inline bool update_cleared(overworld_scene *this)
{
    int i, j;
    bool changed = false;
    /* XXX: Use binary chop */

    for (i = 0; i < this->n_chaps - 1; ++i)
        if (!profile_get_stage(i,
            bekter_at(this->chaps, i, struct chap_rec *)->n_stages - 1)
            ->cleared)
        {
            break;
        }
    this->cleared_chaps = i;

    struct chap_rec *ch = bekter_at(this->chaps, this->cur_chap_idx, struct chap_rec *);
    for (j = 0; j < ch->n_stages - 1; ++j)
        if (!profile_get_stage(this->cur_chap_idx, j)->cleared) break;

    if (j != this->cleared_stages) changed = true;
    this->cleared_stages = j;
    return changed;
}

static void ow_tick(overworld_scene *this, double dt)
{
    floue_tick(this->f, dt);
    double rate = (dt > 0.1 ? 0.1 : dt) * CAM_MOV_FAC;
    update_camera(this, rate);
    this->since_chap_switch += dt;

    if (this->is_in ^ (g_stage == (scene *)this)) {
        this->is_in ^= 1;
        this->since_enter = 0;
    }
    this->since_enter += dt;

    double r1, g1, b1, r2, g2, b2;
    struct chap_rec *ch = bekter_at(this->chaps, this->cur_chap_idx, struct chap_rec *);
    if (this->since_chap_switch <= CHAP_SW_DUR) {
        struct chap_rec *dh = bekter_at(this->chaps, this->last_chap_idx, struct chap_rec *);
        double r = this->since_chap_switch / CHAP_SW_DUR;
        r1 = dh->r1 + r * (ch->r1 - dh->r1);
        g1 = dh->g1 + r * (ch->g1 - dh->g1);
        b1 = dh->b1 + r * (ch->b1 - dh->b1);
        r2 = dh->r2 + r * (ch->r2 - dh->r2);
        g2 = dh->g2 + r * (ch->g2 - dh->g2);
        b2 = dh->b2 + r * (ch->b2 - dh->b2);
    } else {
        r1 = ch->r1; g1 = ch->g1; b1 = ch->b1;
        r2 = ch->r2; g2 = ch->g2; b2 = ch->b2;
    }
    this->f->c0 = (SDL_Color){iround(r1), iround(g1), iround(b1), 255};
    int i;
    for (i = 0; i < 8; ++i)
        this->f->c[i] = (SDL_Color){iround(r2), iround(g2), iround(b2), 255};
}

static inline void draw_stage(overworld_scene *this,
    struct chap_rec *ch, SDL_Texture **tex, int opacity, int i)
{
    int c = (i > this->cleared_stages ? 48 : (i == this->cur_stage_idx) ? 255 : 144);
    SDL_SetTextureColorMod(tex[i], c, c, c);
    SDL_SetTextureAlphaMod(tex[i], opacity);
    SDL_RenderCopy(g_renderer, tex[i], NULL, &(SDL_Rect){
        ((ch->stages[i]->world_c + ch->stages[i]->cam_c1) * PIX_PER_UNIT - this->cam_x) * this->cam_scale + WIN_W / 2,
        ((ch->stages[i]->world_r + ch->stages[i]->cam_r1) * PIX_PER_UNIT - this->cam_y) * this->cam_scale + WIN_H / 2,
        (ch->stages[i]->cam_c2 - ch->stages[i]->cam_c1) * PIX_PER_UNIT * this->cam_scale,
        (ch->stages[i]->cam_r2 - ch->stages[i]->cam_r1) * PIX_PER_UNIT * this->cam_scale
    });
}

static void ow_draw(overworld_scene *this)
{
    floue_draw(this->f);

    if (update_cleared(this)) {
        this->cur_stage_idx = this->cleared_stages;
        move_camera(this);
    }

    int i;
    struct chap_rec *ch = bekter_at(this->chaps, this->cur_chap_idx, struct chap_rec *);
    SDL_Texture **tex = bekter_at(this->stage_tex, this->cur_chap_idx, SDL_Texture **);

    int opacity = 255;
    if (this->since_chap_switch <= CHAP_SW_DUR) {
        double r = this->since_chap_switch / CHAP_SW_DUR;
        opacity = 0;
        if (r < 0.5) {
            ch = bekter_at(this->chaps, this->last_chap_idx, struct chap_rec *);
            tex = bekter_at(this->stage_tex, this->last_chap_idx, SDL_Texture **);
            if (r < 1.0 / 3) opacity = iround(255 * (1 - r * 3));
        } else if (r >= 2.0 / 3) {
            opacity = iround(255 * (r * 3 - 2));
        }
    }

    for (i = 0; i < ch->n_stages; ++i)
        if (i != this->cur_stage_idx) draw_stage(this, ch, tex, opacity, i);
    draw_stage(this, ch, tex, opacity, this->cur_stage_idx);

    double r = this->since_enter / KEYHINT_DUR;
    opacity = this->is_in ?
        (r < 1 ? iround(216 * r) : 216) :
        (r < 1 ? iround(216 * (1 - r)) : 0);
    for (i = 0; i < 4; ++i) {
        this->key_hints[i]->_base.alpha = opacity;
        element_draw((element *)this->key_hints[i]);
    }
}

static void ow_drop(overworld_scene *this)
{
    floue_drop(this->f);

    int i, j;

    SDL_Texture **q;
    for bekter_each(this->stage_tex, i, q) {
        int n = bekter_at(this->chaps, i / sizeof(struct chap_rec *), struct chap_rec *)->n_stages;
        for (j = 0; j < n; ++j) SDL_DestroyTexture(q[j]);
        free(q);
    }
    bekter_drop(this->stage_tex);

    struct chap_rec *p;
    for bekter_each(this->chaps, i, p) chap_drop(p);
    bekter_drop(this->chaps);

    for (i = 0; i < 4; ++i) element_drop(this->key_hints[i]);
}

static void ow_key(overworld_scene *this, SDL_KeyboardEvent *ev)
{
    if (ev->state != SDL_PRESSED) return;
    switch (ev->keysym.sym) {
        case SDLK_ESCAPE:
            g_stage = transition_slideup_create(&g_stage, this->bg, 0.5);
            break;
        case SDLK_SPACE:
        case SDLK_RETURN:
            g_stage = (scene *)overworld_menu_create(this);
            break;
        case SDLK_LEFT:
            if (this->cur_stage_idx > 0) {
                this->cur_stage_idx--;
                move_camera(this);
            }
            break;
        case SDLK_RIGHT:
            if (this->cur_stage_idx < this->cleared_stages) {
                this->cur_stage_idx++;
                move_camera(this);
            }
            break;
        case SDLK_UP:
            if (this->cur_chap_idx > 0) {
                this->last_chap_idx = this->cur_chap_idx;
                this->cur_chap_idx--;
                update_cleared(this);
                this->cur_stage_idx = min(this->cur_stage_idx, this->cleared_stages);
                move_camera(this);
                this->since_chap_switch = 0;
            }
            break;
        case SDLK_DOWN:
            if (this->cur_chap_idx < this->cleared_chaps) {
                this->last_chap_idx = this->cur_chap_idx;
                this->cur_chap_idx++;
                update_cleared(this);
                this->cur_stage_idx = min(this->cur_stage_idx, this->cleared_stages);
                move_camera(this);
                this->since_chap_switch = 0;
            }
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
    return (iround((double)rs / ct) << 24) |
        (iround((double)gs / ct) << 16) |
        (iround((double)bs / ct) << 8) |
        (iround((double)raw_a / (SAMPLER_TEX_SZ * SAMPLER_TEX_SZ / 4)));
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
    ch->idx = this->n_chaps;
    bekter_pushback(this->chaps, ch);
    this->n_chaps++;

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
    ret->f = floue_create((SDL_Color){255, 255, 255, 255});
    int i;
    for (i = 0; i < 8; ++i)
        floue_add(ret->f, (SDL_Point){rand() % WIN_W, rand() % WIN_H},
            (SDL_Color){192, 192, 192, 255}, rand() % (WIN_W / 4) + WIN_W / 4,
            (double)(i + 5) / 16);

    ret->n_chaps = 0;
    ret->chaps = bekter_create();
    ret->stage_tex = bekter_create();
    load_chapter(ret, "chap.csv");
    load_chapter(ret, "chap2.csv");

    ret->cur_chap_idx = 0;
    ret->cur_stage_idx = 0;
    update_cleared(ret);

    move_camera(ret);
    ret->cam_x = ret->cam_targx;
    ret->cam_y = ret->cam_targy;
    ret->cam_scale = ret->cam_targscale;

    ret->since_chap_switch = CHAP_SW_DUR * 2;

    label *l = label_create(FONT_UPRIGHT, 24,
        (SDL_Color){0, 0, 0}, WIN_W, "");
    label_set_keyed_text(l, " ..`.  ..`.  Select chapter", "^v");
    element_place_anchored((element *)l,
        WIN_W * 0.05, WIN_H * 0.775, 0, 0.5);
    ret->key_hints[0] = l;

    l = label_create(FONT_UPRIGHT, 24,
        (SDL_Color){0, 0, 0}, WIN_W, "");
    label_set_keyed_text(l, " ..`.  ..`.  Select stage", "<>");
    element_place_anchored((element *)l,
        WIN_W * 0.05, WIN_H * 0.825, 0, 0.5);
    ret->key_hints[1] = l;

    l = label_create(FONT_UPRIGHT, 24,
        (SDL_Color){0, 0, 0}, WIN_W, "");
    label_set_keyed_text(l, " ..`.  Confirm", "\\");
    element_place_anchored((element *)l,
        WIN_W * 0.05, WIN_H * 0.875, 0, 0.5);
    ret->key_hints[2] = l;

    l = label_create(FONT_UPRIGHT, 24,
        (SDL_Color){0, 0, 0}, WIN_W, "");
    label_set_keyed_text(l, " ..`.  Back", "~");
    element_place_anchored((element *)l,
        WIN_W * 0.05, WIN_H * 0.925, 0, 0.5);
    ret->key_hints[3] = l;

    return ret;
}
