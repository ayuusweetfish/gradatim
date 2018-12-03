#include "gameplay.h"

#include "global.h"
#include "bekter.h"
#include "game_data.h"
#include "unary_transition.h"
#include "pause.h"
#include "dialogue.h"
#include "chapfin.h"

#include <math.h>

static const double UNIT_PX = 48;
static const double WIN_W_UNITS = (double)WIN_W / UNIT_PX;
static const double WIN_H_UNITS = (double)WIN_H / UNIT_PX;
static const double SPR_SCALE = 3;

#define AUD_OFFSET  (-this->chap->offs + 0.04)
#define BEAT        (this->chap->beat)
#define HOP_SPD SIM_GRAVITY
static const double HOP_PRED_DUR = 0.2;
static const double HOP_GRACE_DUR = 0.15;
#define ANTHOP_DELUGE_SPD (6.0 * SIM_GRAVITY)
static const double HOR_SPD = 4;
static const double DASH_DUR = 1;
#define DASH_HOR_V0     (6.5 * 1.414213562)
#define DASH_HOR_ACCEL  (DASH_HOR_V0 * DASH_DUR)
#define DASH_VER_V0     (5.5 * 1.414213562)
#define DASH_VER_ACCEL  (DASH_VER_V0 * DASH_DUR - SIM_GRAVITY)
static const double DASH_MIN_DUR = 0.95;    /* TODO: Keep sync with overall toleration */
static const double DASH_DIAG_SCALE = 0.8;

static const double CAM_MOV_FAC = 8;
static const double STAGE_TRANSITION_DUR = 2;

static const double LEADIN_INIT = 1;
static const double LEADIN_DUR = 0.4; /* Seconds */
static const double FAILURE_SPF = 0.1;
static const double STRETTO_RANGE = 2.5;

/* Short for metronome - too long! */
static const int MT_PADDING = 12;
static const int MT_WEIGHT = 8;
static const SDL_Rect MT_DOWNBEAT[4] = {
    (SDL_Rect){
        MT_PADDING, MT_PADDING,
        WIN_W - MT_PADDING * 2, MT_WEIGHT
    }, (SDL_Rect) {
        MT_PADDING, MT_PADDING + MT_WEIGHT,
        MT_WEIGHT, WIN_H - MT_PADDING * 2 - MT_WEIGHT * 2
    }, (SDL_Rect) {
        MT_PADDING, WIN_H - MT_PADDING - MT_WEIGHT,
        WIN_W - MT_PADDING * 2, MT_WEIGHT
    }, (SDL_Rect) {
        WIN_W - MT_PADDING - MT_WEIGHT, MT_PADDING + MT_WEIGHT,
        MT_WEIGHT, WIN_H - MT_PADDING * 2 - MT_WEIGHT * 2
    }
};
static const SDL_Rect MT_UPBEAT[2] = {
    (SDL_Rect){
        MT_PADDING, MT_PADDING,
        WIN_W - MT_PADDING * 2, WIN_H - MT_PADDING * 2
    }, (SDL_Rect){
        MT_PADDING + MT_WEIGHT, MT_PADDING + MT_WEIGHT,
        WIN_W - MT_PADDING * 2 - MT_WEIGHT * 2,
        WIN_H - MT_PADDING * 2 - MT_WEIGHT * 2
    }
};

static inline double clamp(double x, double l, double u)
{
    return (x < l ? l : (x > u ? u : x));
}

static inline double get_audio_position(gameplay_scene *this)
{
    double sec = (double)orion_tell(&g_orion, TRACKID_STAGE_BGM) / 44100;
    return (sec + AUD_OFFSET) / BEAT;
}

static inline void switch_stage_ctx(gameplay_scene *this)
{
    if (++this->cur_stage_idx == this->chap->n_stages) {
        this->disp_state = DISP_CHAPFIN;
    } else {
        this->rec = this->chap->stages[this->cur_stage_idx];
        this->simulator = stage_create_sim(this->rec);
    }
}

static inline void update_camera(gameplay_scene *this, double rate)
{
    double dest_x = clamp(this->simulator->prot.x,
        this->rec->cam_c1 + WIN_W_UNITS / 2, this->rec->cam_c2 - WIN_W_UNITS / 2);
    double dest_y = clamp(this->simulator->prot.y,
        this->rec->cam_r1 + WIN_H_UNITS / 2, this->rec->cam_r2 - WIN_H_UNITS / 2);
    double cam_dx = dest_x - (this->cam_x + WIN_W_UNITS / 2);
    double cam_dy = dest_y - (this->cam_y + WIN_H_UNITS / 2);
    this->cam_x += rate * cam_dx;
    this->cam_y += rate * cam_dy;
}

static inline void stop_prot(gameplay_scene *this)
{
    this->hor_state = HOR_STATE_NONE;
    this->ver_state = VER_STATE_NONE;
    this->simulator->prot.tag = 0;
}

static void retry_reinit(gameplay_scene *this)
{
    stop_prot(this);
    this->simulator->prot.x = this->rec->spawn_c;
    this->simulator->prot.y = this->rec->spawn_r;
    this->facing = HOR_STATE_RIGHT;
    this->disp_state = DISP_NORMAL;
    sim_reinit(this->simulator);
    this->simulator->cur_time = get_audio_position(this) - this->aud_sim_offset;
    this->dialogue_triggered = 0;
    update_camera(this, 1);
}

static void gameplay_scene_tick(gameplay_scene *this, double dt)
{
    if (this->disp_state == DISP_LEADIN) {
        if ((this->disp_time -= dt) <= 0) {
            this->disp_state = DISP_NORMAL;
            SDL_DestroyTexture(this->leadin_tex);
            this->leadin_tex = NULL;
            this->aud_sim_offset /= this->aud_sim_offset_n_samples;
            this->simulator->cur_time =
                get_audio_position(this) - this->aud_sim_offset;
        } else {
            return;
        }
    } else if (this->disp_state == DISP_FAILURE) {
        if ((this->disp_time -= dt) <= 0) {
            /* Run a transition to reset the stage */
            if (g_stage == (scene *)this)
                g_stage = (scene *)utransition_fade_create(
                    &g_stage, 1, (utransition_callback)retry_reinit);
        }
        return;
    } else if (this->disp_state == DISP_CHAPFIN) {
        if (g_stage == (scene *)this)
            g_stage = (scene *)chapfin_scene_create(this);
        stop_prot(this);
    }

    switch (this->simulator->prot.tag) {
        case PROT_TAG_FAILURE:
            /* Failure */
            this->disp_state = DISP_FAILURE;
            this->disp_time = FAILURE_SPF * FAILURE_NF;
            this->simulator->prot.tag = 0;
            break;
        case PROT_TAG_NXSTAGE:
            /* Move on to the next stage */
            if (this->prev_sim == NULL) {
                this->prev_sim = this->simulator;
                switch_stage_ctx(this);
                this->simulator->cur_time = this->prev_sim->cur_time;
                this->simulator->prot = this->prev_sim->prot;
                int delta_x = this->simulator->worldc - this->prev_sim->worldc;
                int delta_y = this->simulator->worldr - this->prev_sim->worldr;
                this->simulator->prot.x -= delta_x;
                this->simulator->prot.y -= delta_y;
                this->cam_x -= delta_x;
                this->cam_y -= delta_y;
            } else if (this->simulator->cur_time - this->simulator->prot.t
                >= STAGE_TRANSITION_DUR)
            {
                sim_drop(this->prev_sim);
                this->prev_sim = NULL;
                this->simulator->prot.tag = 0;
            }
            break;
        case PROT_TAG_REFILL:
            puts("Refill");
            this->simulator->prot.tag = 0;
            break;
    }

    this->simulator->prot.ay =
        (this->ver_state == VER_STATE_DOWN) ? 4.0 * SIM_GRAVITY : 0;
    double plunge_vy = this->simulator->prot.ay;
    if (this->mov_state == MOV_ANTHOP) {
        if (this->mov_time <= 0) {
            /* Perform a jump */
            this->mov_state = MOV_NORMAL;
            this->simulator->prot.vy = -HOP_SPD;
        } else {
            /* Deluging */
            this->simulator->prot.ay = ANTHOP_DELUGE_SPD;
        }
        this->mov_time -= dt / BEAT;
        this->simulator->prot.vx = 0;
    } else if (this->mov_state & MOV_DASH_BASE) {
        if (this->mov_time <= 0) {
            /* XXX: Avoid duplicates? */
            this->mov_state = MOV_NORMAL;
            this->simulator->prot.ax = 0;
            this->simulator->prot.ay = 0;
        } else {
            if (this->mov_state & MOV_HORDASH & 3) {
                this->simulator->prot.ax =
                    (this->mov_state & MOV_DASH_LEFT) ?
                    +DASH_HOR_ACCEL : -DASH_HOR_ACCEL;
                this->simulator->prot.ay = -SIM_GRAVITY;
            }
            if (this->mov_state & MOV_VERDASH & 3) {
                /* If a diagonal dash is taking place,
                 * the vertical acceleration is overridden here */
                this->simulator->prot.ay =
                    (this->mov_state & MOV_DASH_UP) ? DASH_VER_ACCEL : 0;
            }
            if (this->mov_state & 3) {
                this->simulator->prot.ax *= DASH_DIAG_SCALE;
                this->simulator->prot.ay *= DASH_DIAG_SCALE;
            }
            /* In case the protagonist runs into something,
             * the velocity becomes 0 and should not change any more */
            if (this->simulator->prot.vx * this->simulator->prot.ax >= 0)
                this->simulator->prot.ax = 0;
            if (this->simulator->prot.vy * this->simulator->prot.ay >= 0)
                this->simulator->prot.ay = 0;
            /* In case a plunge is in progress, its speed change
             * should be taken into account */
            this->simulator->prot.ay += plunge_vy;
        }
        this->mov_time -= dt / BEAT;
    } else {
        /* Normal state */
        this->simulator->prot.vx = 0;
    }
    double hor_mov_vx =
        (this->hor_state == HOR_STATE_LEFT) ? -HOR_SPD :
        (this->hor_state == HOR_STATE_RIGHT) ? +HOR_SPD : 0;
    this->simulator->prot.vx += hor_mov_vx;

    double rt = this->rem_time + dt / (BEAT * this->chap->beat_mul);
    while (rt >= SIM_STEPLEN) {
        sim_tick(this->simulator);
        rt -= SIM_STEPLEN;
    }
    this->rem_time = rt;

    /* Horizontal speed should be cancelled out immediately
     * However, not if such will result in a 'bounce' */
    if (this->simulator->prot.vx * (this->simulator->prot.vx - hor_mov_vx) > 0)
        this->simulator->prot.vx -= hor_mov_vx;

    /* Check for dialogues */
    int i;
    int px = (int)this->simulator->prot.x,
        py = (int)this->simulator->prot.y;
    if (!(this->mods & MOD_SEMPLICE)) for (i = 0; i < this->rec->plot_ct; ++i) {
        stage_dialogue *d = bekter_at_ptr(this->rec->plot, i, stage_dialogue);
        if (!(this->dialogue_triggered & (1 << i)) &&
            px >= d->c1 && px <= d->c2 && py >= d->r1 && py <= d->r2)
        {
            this->mov_state = MOV_NORMAL;
            this->ver_state = VER_STATE_NONE;
            this->hor_state = HOR_STATE_NONE;
            this->simulator->prot.ax = this->simulator->prot.ay =
            this->simulator->prot.vx = this->simulator->prot.vy = 0;
            this->dialogue_triggered |= (1 << i);
            g_stage = (scene *)dialogue_create(&g_stage, d->content);
        }
    }

    /* Move the camera */
    double rate = (dt > 0.1 ? 0.1 : dt) * CAM_MOV_FAC;
    update_camera(this, rate);
}

static inline texture get_texture(gameplay_scene *this, sobj *o)
{
    return o->tag == OBJID_DISPONLY ?
        retrieve_texture(bekter_at(this->rec->strtab, (int)
            ((int)(this->simulator->cur_time / o->t) % 2 == 0 ? o->vx : o->vy),
        char *)) : this->rec->grid_tex[o->tag];
}

static inline void render_objects(gameplay_scene *this,
    bool is_prev, bool is_after, double offsx, double offsy)
{
    sim *sim = (is_prev ? this->prev_sim : this->simulator);
    double cx = (is_prev ? this->cam_x + offsx : this->cam_x);
    double cy = (is_prev ? this->cam_y + offsy : this->cam_y);
    int rmin = clamp(floorf(cy), 0, sim->grows),
        rmax = clamp(ceilf(cy + WIN_H_UNITS), 0, sim->grows),
        cmin = clamp(floorf(cx), 0, sim->gcols),
        cmax = clamp(ceilf(cx + WIN_W_UNITS), 0, sim->gcols);
    int r, c;
    int cxi = round(cx * UNIT_PX),
        cyi = round(cy * UNIT_PX);
    for (r = rmin; r < rmax; ++r)
        for (c = cmin; c < cmax; ++c) {
            sobj *o = &sim_grid(sim, r, c);
            if (o->tag != 0) {
                render_texture_scaled(get_texture(this, o),
                    (int)o->x * UNIT_PX - cxi,
                    (int)o->y * UNIT_PX - cyi,
                    SPR_SCALE
                );
            }
        }
    for (r = 0; r < sim->anim_sz; ++r) {
        sobj *o = sim->anim[r];
        render_texture_scaled(get_texture(this, o),
            round((o->x + o->tx) * UNIT_PX) - cxi,
            round((o->y + o->ty) * UNIT_PX) - cyi,
            SPR_SCALE
        );
    }
}

void gameplay_run_leadin(gameplay_scene *this)
{
    this->disp_state = DISP_LEADIN;
    this->disp_time = LEADIN_DUR + LEADIN_INIT;

    this->leadin_tex = SDL_CreateTexture(
        g_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
        WIN_W, WIN_H);
    SDL_SetTextureBlendMode(this->leadin_tex, SDL_BLENDMODE_BLEND);
}

static inline void draw_overlay(gameplay_scene *this)
{
    double beats = get_audio_position(this);
    if (this->disp_state == DISP_LEADIN) {
        this->aud_sim_offset = beats - this->simulator->cur_time;
        this->aud_sim_offset_n_samples++;
        return;
    }
    if (this->mods & MOD_SOTTO_VOCO) return;
    if (beats < -1./16) return;
    int beats_i = (int)(beats + 1./16);
    double beats_d = beats - beats_i;
    beats_i %= this->chap->sig;
    beats_d /= this->chap->beat_mul;
    bool is_downbeat = this->chap->dash_mask & (1 << beats_i),
        is_upbeat = this->chap->hop_mask & (1 << beats_i);
    if (!is_upbeat) return;
    int opacity = round((
        beats_d < 0 ? (1 + beats_d * 16) :
        beats_d < 0.5 ? (1 - beats_d * 2) : 0) * 255);
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, opacity);
    if (is_downbeat)
        SDL_RenderFillRects(g_renderer, MT_DOWNBEAT, 4);
    else
        SDL_RenderDrawRects(g_renderer, MT_UPBEAT, 2);
}

static inline void flashlight_texture(SDL_Texture *tex,
    int w, int h, double x, double y, double rsqr, double rosqr)
{
    int i, j;
    void *pixdata;
    int pitch;
    SDL_LockTexture(tex, NULL, &pixdata, &pitch);
    for (j = 0; j < h; ++j)
        for (i = 0; i < w; ++i) {
            double r = sqr(i - x) + sqr(j - y);
            if (r <= rsqr) {
                *((int *)(pixdata + j * pitch) + i) = 0;
            } else if (r <= rosqr) {
                int a = round(255 * (r - rsqr) / (rosqr - rsqr));
                *((int *)(pixdata + j * pitch) + i) = a;
            } else {
                *((int *)(pixdata + j * pitch) + i) = 255;
            }
        }
    SDL_UnlockTexture(tex);
}

static inline void pause_sound(gameplay_scene *this)
{
    int i;
    for (i = 0; i < this->chap->n_tracks; ++i)
        orion_pause(&g_orion, TRACKID_STAGE_BGM + i);
}

static inline void update_sound(gameplay_scene *this)
{
    int i;
    if ((scene *)this == g_stage) {
        for (i = 0; i < this->chap->n_tracks; ++i)
            orion_resume(&g_orion, TRACKID_STAGE_BGM + i);
    }
    if (this->mods & MOD_A_CAPELLA) {
        for (i = 0; i < this->chap->n_tracks; ++i)
            orion_ramp(&g_orion, TRACKID_STAGE_BGM + i, 0, 0);
    } else {
        float val[MAX_CHAP_TRACKS] = { 0 }, sum = 0;
        for (i = 0; i < this->rec->aud_ct; ++i) {
            float d = sqrtf(
                sqr(this->simulator->prot.y + this->rec->world_r - this->rec->aud[i].r) +
                sqr(this->simulator->prot.x + this->rec->world_c - this->rec->aud[i].c)
            );
            d = 1 / d;
            val[this->rec->aud[i].tid] += d;
            sum += d;
        }
        for (i = 0; i < this->chap->n_tracks; ++i)
            orion_ramp(&g_orion, TRACKID_STAGE_BGM + i, 0.03, val[i] / sum);
    }
}

static void gameplay_scene_draw(gameplay_scene *this)
{
    update_sound(this);

    SDL_SetRenderDrawColor(g_renderer, 216, 224, 255, 255);
    SDL_RenderClear(g_renderer);

    /* Draw the previous stage during transition */
    if (this->prev_sim != NULL) {
        int delta_x = this->simulator->worldc - this->prev_sim->worldc;
        int delta_y = this->simulator->worldr - this->prev_sim->worldr;
        render_objects(this, true, false, delta_x, delta_y);
        render_objects(this, true, true, delta_x, delta_y);
    }

    render_objects(this, false, false, 0, 0);

    double prot_disp_x = (this->simulator->prot.x - this->cam_x) * UNIT_PX;
    double prot_disp_y = (this->simulator->prot.y - this->cam_y) * UNIT_PX;
    double prot_w = this->simulator->prot.w * UNIT_PX;
    double prot_h = this->simulator->prot.h * UNIT_PX;

    texture prot_tex = this->rec->prot_tex;
    if (this->disp_state == DISP_FAILURE) {
        int f_idx = clamp(FAILURE_NF - (int)(this->disp_time / FAILURE_SPF) - 1,
            0, FAILURE_NF - 1);
        prot_tex = this->rec->prot_fail_tex[f_idx];
        /* The failure animation should be displayed above everything else */
        render_objects(this, false, true, 0, 0);
        prot_disp_x -= (prot_tex.range.w * SPR_SCALE - prot_w) / 2;
        prot_disp_y -= (prot_tex.range.h * SPR_SCALE - prot_h) / 2;
        prot_w = prot_tex.range.w * SPR_SCALE;
        prot_h = prot_tex.range.h * SPR_SCALE;
    }

    render_texture_ex(prot_tex, &(SDL_Rect){
        prot_disp_x, prot_disp_y, round(prot_w), round(prot_h),
    }, 0, NULL, (this->facing == HOR_STATE_LEFT ? SDL_FLIP_HORIZONTAL : 0));

    if (this->disp_state != DISP_FAILURE)
        render_objects(this, false, true, 0, 0);

    prot_disp_x += this->simulator->prot.w / 2 * UNIT_PX;
    prot_disp_y += this->simulator->prot.h / 2 * UNIT_PX;
    if (this->disp_state == DISP_LEADIN) {
        double radius = 0, radius_o = 0;
        if (this->disp_time <= LEADIN_DUR) {
            if (this->mods & MOD_STRETTO)
                radius = UNIT_PX * (1 + (STRETTO_RANGE - 1) *
                    ease_quad_inout(1 - this->disp_time / LEADIN_DUR));
            else radius = UNIT_PX + WIN_W * (1 - this->disp_time / LEADIN_DUR);
            radius_o = radius + UNIT_PX * 0.5;
        } else {
            double t = this->disp_time - LEADIN_DUR;
            double p = ease_elastic_out(1 - t / LEADIN_INIT, 0.3);
            radius_o = p * UNIT_PX * 1.5;
            radius = radius_o - UNIT_PX * 0.5;
            if (radius < 0) radius = 0;
        }
        flashlight_texture(this->leadin_tex,
            WIN_W, WIN_H, prot_disp_x, prot_disp_y, sqr(radius), sqr(radius_o));
        SDL_RenderCopy(g_renderer, this->leadin_tex, NULL, NULL);
    } else if (this->mods & MOD_STRETTO) {
        /* Flashlight! */
        if (this->leadin_tex == NULL) {
            this->leadin_tex = SDL_CreateTexture(
                g_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                WIN_W * 2, WIN_H * 2);
            SDL_SetTextureBlendMode(this->leadin_tex, SDL_BLENDMODE_BLEND);
            flashlight_texture(this->leadin_tex,
                WIN_W * 2, WIN_H * 2, WIN_W, WIN_H,
                sqr(UNIT_PX * STRETTO_RANGE), sqr(UNIT_PX * (STRETTO_RANGE + 0.5)));
        }
        SDL_RenderCopy(g_renderer, this->leadin_tex, &(SDL_Rect) {
            round(WIN_W - prot_disp_x), round(WIN_H - prot_disp_y),
            WIN_W, WIN_H
        }, NULL);
    }

    draw_overlay(this);
}

static void gameplay_scene_drop(gameplay_scene *this)
{
    if (this->leadin_tex != NULL) SDL_DestroyTexture(this->leadin_tex);
    if (this->prev_sim != NULL) sim_drop(this->prev_sim);
    if (this->simulator != NULL && this->simulator != this->prev_sim)
        sim_drop(this->simulator);
    pause_sound(this);
}

static void try_hop(gameplay_scene *this)
{
    if ((this->simulator->cur_time - this->simulator->last_land) <= HOP_GRACE_DUR) {
        /* Grace jump */
        this->simulator->prot.vy = -HOP_SPD;
    } else if (this->simulator->prot.tag == PROT_TAG_PUFF &&
        (this->simulator->cur_time - this->simulator->prot.t) <= HOP_GRACE_DUR)
    {
        /* On a puff */
        this->simulator->prot.vy = -HOP_SPD;
    } else if (sim_prophecy(this->simulator, HOP_PRED_DUR)) {
        /* Will land soon, plunge and then jump */
        this->mov_state = MOV_ANTHOP;
        /* Binary search on time needed till landing happens */
        double lo = 0, hi = HOP_PRED_DUR, mid;
        int i;
        for (i = 0; i < 10; ++i) {
            mid = (lo + hi) / 2;
            if (sim_prophecy(this->simulator, mid)) hi = mid;
            else lo = mid;
        }
        this->mov_time = hi;
    }
}

static void try_dash(gameplay_scene *this)
{
    /* In case of direction updates, the time should not be reset */
    double dur = (this->mov_state & MOV_DASH_BASE) ? this->mov_time : DASH_DUR;
    if (dur < DASH_MIN_DUR) return;
    int dir_has = 0, dir_denotes = 0;
    if (this->ver_state == VER_STATE_UP) {
        dir_has |= 2;
        dir_denotes |= MOV_DASH_UP;
        this->simulator->prot.vy = -DASH_VER_V0 * dur;
    }
    if (this->hor_state != HOR_STATE_NONE || dir_has == 0) {
        int s = (this->hor_state == HOR_STATE_NONE ?
            this->facing : this->hor_state);
        dir_has |= 1;
        dir_denotes |= (s == HOR_STATE_LEFT ? MOV_DASH_LEFT : 0);
        this->simulator->prot.vx =
            (s == HOR_STATE_LEFT ? -DASH_HOR_V0 : +DASH_HOR_V0) * dur;
    }
    if (dir_has == 3) {
        this->simulator->prot.vx *= DASH_DIAG_SCALE;
        this->simulator->prot.vy *= DASH_DIAG_SCALE;
    }
    this->simulator->last_land = -1e10; /* Disable grace jumps */
    this->mov_state = MOV_DASH_BASE | dir_has | dir_denotes;
    this->mov_time = dur;
}

static void gameplay_scene_key_handler(gameplay_scene *this, SDL_KeyboardEvent *ev)
{
#define toggle(__thisstate, __keystate, __has, __none) do { \
    if ((__keystate) == SDL_PRESSED) (__thisstate) = (__has); \
    else if ((__thisstate) == (__has)) (__thisstate) = (__none); \
} while (0)

    if (ev->repeat) return;
    if (this->disp_state != DISP_NORMAL) return;
    switch (ev->keysym.sym) {
        case SDLK_c:
            if (ev->state == SDL_PRESSED) try_hop(this);
            break;
        case SDLK_x:
            if (ev->state == SDL_PRESSED) try_dash(this);
            break;
        case SDLK_UP:
            toggle(this->ver_state, ev->state, VER_STATE_UP, VER_STATE_NONE);
            if (ev->state == SDL_PRESSED && (this->mov_state & MOV_DASH_BASE))
                try_dash(this);
            break;
        case SDLK_DOWN:
            toggle(this->ver_state, ev->state, VER_STATE_DOWN, VER_STATE_NONE);
            if (ev->state == SDL_PRESSED && (this->mov_state & MOV_DASH_BASE))
                try_dash(this);
            break;
        case SDLK_LEFT:
            if (ev->state == SDL_PRESSED) this->facing = HOR_STATE_LEFT;
            toggle(this->hor_state, ev->state, HOR_STATE_LEFT, HOR_STATE_NONE);
            if (ev->state == SDL_PRESSED && (this->mov_state & MOV_DASH_BASE))
                try_dash(this);
            break;
        case SDLK_RIGHT:
            if (ev->state == SDL_PRESSED) this->facing = HOR_STATE_RIGHT;
            toggle(this->hor_state, ev->state, HOR_STATE_RIGHT, HOR_STATE_NONE);
            if (ev->state == SDL_PRESSED && (this->mov_state & MOV_DASH_BASE))
                try_dash(this);
            break;
        case SDLK_ESCAPE:
            if (ev->state == SDL_PRESSED) {
                g_stage = (scene *)pause_scene_create(
                    &g_stage, (retry_callback)retry_reinit, this->bg);
                pause_sound(this);
            }
            break;
        default: break;
    }
}

gameplay_scene *gameplay_scene_create(scene *bg, struct chap_rec *chap, int idx, int mods)
{
    gameplay_scene *ret = malloc(sizeof(gameplay_scene));
    memset(ret, 0, sizeof(gameplay_scene));
    ret->_base.children = bekter_create();
    ret->_base.tick = (scene_tick_func)gameplay_scene_tick;
    ret->_base.draw = (scene_draw_func)gameplay_scene_draw;
    ret->_base.drop = (scene_drop_func)gameplay_scene_drop;
    ret->_base.key_handler = (scene_key_func)gameplay_scene_key_handler;
    ret->bg = bg;

    ret->rem_time = 0;
    ret->facing = HOR_STATE_RIGHT;

    if (mods & MOD_STRETTO) mods |= MOD_SEMPLICE;
    ret->mods = mods;

    ret->prev_sim = NULL;
    ret->chap = chap;
    ret->cur_stage_idx = idx - 1;
    switch_stage_ctx(ret);

    ret->cam_x = clamp(ret->simulator->prot.x,
        WIN_W_UNITS / 2, ret->simulator->gcols - WIN_W_UNITS / 2) - WIN_W_UNITS / 2;
    ret->cam_y = clamp(ret->simulator->prot.y,
        WIN_H_UNITS / 2, ret->simulator->grows - WIN_H_UNITS / 2) - WIN_H_UNITS / 2;

    int i;
    for (i = 0; i < chap->n_tracks; ++i) {
        if (chap->tracks[i].src_id == -1) {
            orion_load_ogg(&g_orion, TRACKID_STAGE_BGM + i, chap->tracks[i].str);
        } else if (strcmp(chap->tracks[i].str, "lowpass") == 0) {
            orion_apply_lowpass(&g_orion,
                TRACKID_STAGE_BGM + chap->tracks[i].src_id,
                TRACKID_STAGE_BGM + i,
                chap->tracks[i].arg);
        }
    }
    /* Play at the same time in order to avoid incorrect syncronization */
    for (i = 0; i < chap->n_tracks; ++i) {
        orion_play_loop(&g_orion, TRACKID_STAGE_BGM + i,
            0,
            (int)(chap->offs * 44100),
            (int)((chap->offs + chap->beat * chap->loop) * 44100));
        orion_ramp(&g_orion, TRACKID_STAGE_BGM + i, 0, 0);
        orion_pause(&g_orion, TRACKID_STAGE_BGM + i);
    }

    return ret;
}
