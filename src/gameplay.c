#include "gameplay.h"

#include "global.h"
#include "bekter.h"
#include "game_data.h"
#include "unary_transition.h"
#include "pause.h"
#include "dialogue.h"
#include "profile_data.h"
#include "chapfin.h"

#include <math.h>
#include <stdlib.h>

#define lerp(__x, __a, __b) ((__a) + (__x) * ((__b) - (__a)))

static const double UNIT_PX = 64;
static const double WIN_W_UNITS = (double)WIN_W / UNIT_PX;
static const double WIN_H_UNITS = (double)WIN_H / UNIT_PX;
static const double SPR_PX = 16;
static const double SPR_SCALE = UNIT_PX / SPR_PX;

static const double VIVACE_MUL = 1.2;
static const double ANDANTE_MUL = 0.8;

#define AUD_OFFSET  (-this->chap->offs / this->mul + 0.04)
#define BEAT        (this->chap->beat / this->mul)
#define HOP_SPD SIM_GRAVITY
static const double HOP_PRED_DUR = 0.3;
static const double HOP_GRACE_DUR = 0.2;
#define ANTHOP_DELUGE_SPD (10.0 * SIM_GRAVITY)
static const double HOR_SPD = 4;
static const double DASH_DUR = 1;
#define DASH_HOR_V0     (6.5 * 1.414213562)
#define DASH_HOR_ACCEL  (DASH_HOR_V0 * DASH_DUR)
#define DASH_VER_V0     (5.5 * 1.414213562)
#define DASH_VER_ACCEL  (DASH_VER_V0 * DASH_DUR - SIM_GRAVITY)
static const double HOP_TOLERANCE = 1./4;
static const double DASH_TOLERANCE = 1./3;
static const double DASH_MIN_DUR = 1 - DASH_DUR * DASH_TOLERANCE;
static const double DASH_DIAG_SCALE = 0.8;
static const double REFILL_PERSISTENCE = 2; /* In beats */
static const int AV_OFFSET_INTV = 60;

static const double CAM_MOV_FAC = 8;
static const double STAGE_TRANSITION_DUR = 2;

static const double LEADIN_INIT = 1;
static const double LEADIN_DUR = 0.4; /* Seconds */
static const double FAILURE_SPF = 0.1;
static const double STRETTO_RANGE = 2.5;
static const double DIALOGUE_ZOOM_DUR = 0.9;
static const double DIALOGUE_ZOOM_SCALE = 3;
static const int HINT_FONTSZ = 36;
static const int HINT_PADDING = 12;
static const int CLOCK_CHAP_FONTSZ = 44;
static const int CLOCK_STG_FONTSZ = 32;
static const double CLOCK_BLINK_DUR = 2;

/* Short for metronome - too long! */
static const int MT_PADDING = 8;
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

static inline double get_audio_position(gameplay_scene *this)
{
    double sec = (double)orion_tell(&g_orion, TRACKID_STAGE_BGM) / 44100;
    return (sec + AUD_OFFSET + profile.av_offset * 0.001) / BEAT;
}

/* `cant_*` functions eturn 0 if normal,
    1 if early, 2 if late, 3 if not at the beat, and
    (dash only) 4 if using a refill */

static inline char cant_hop(gameplay_scene *this)
{
    if (this->mods & MOD_A_PIACERE) return 0;
    double b = get_audio_position(this);
    int i = iround(b);
    return
        (this->chap->hop_mask & (1 << (i % this->chap->sig))) ?
        (fabs(b - i) <= HOP_TOLERANCE * this->chap->beat_mul ?
            0 : (b < i ? 1 : 2)) : 3;
}

static inline char cant_dash(gameplay_scene *this, bool is_dirchg)
{
    if ((this->mods & MOD_A_PIACERE) || is_dirchg) return 0;
    if (this->refill_time >= 0 && !cant_hop(this)) {
        this->refill_time = -1;
        return 4;
    }
    double b = get_audio_position(this);
    int i = iround(b);
    int mask = (this->mods & MOD_RUBATO) ?
        this->chap->hop_mask : this->chap->dash_mask;
    return
        (this->chap->dash_mask & (1 << (i % this->chap->sig))) ?
        (fabs(b - i) <= DASH_TOLERANCE * this->chap->beat_mul ?
            0 : (b < i ? 1 : 2)) : 3;
}

/*
 * type == 0: Normal
 * type == 1: Early (blue)
 * type == 2: Late (red)
 * type == 3: Not at the beat (grey)
 */
static inline void add_hop_particles(gameplay_scene *this, char type)
{
    int i, n = (type == 0 ? 5 : 3);
    for (i = 0; i < n; ++i) {
        int sz = i % 2 == 0 ? SPR_SCALE * 2 : SPR_SCALE;
        int r = 0, g = 0, b = 0;
        if (type == 0) {
            r = g = b = 255 - rand() % 16;
        } else if (type == 1) {
            r = 168 - rand() % 16;
            g = 216 - rand() % 16;
            b = 255;
        } else if (type == 2) {
            g = b = 192 - rand() % 16;
            r = 255;
        } else if (type == 3) {
            r = g = b = 216 - rand() % 16;
        }
        particle_add(&this->particle,
            this->simulator->prot.x * UNIT_PX + rand() % 31 - 15,
            this->simulator->prot.y * UNIT_PX + rand() % 31 - 15,
            -this->simulator->prot.vx * UNIT_PX / 6 + rand() % 31 - 15,
            UNIT_PX * 2 + rand() % 31 - 15,
            sz, sz, 0.2, 0.5, r, g, b);
    }
}

/*
 * type == 0: Normal
 * type == 1: Using refill (green)
 * type == 2: Tail
 * type == 3: Tail, using refill
 * type == 4: Afterglow
 * type == 5: Afterglow, using refill
 */
static inline void add_dash_particles(gameplay_scene *this, char type)
{
    int i, n = (type >= 2 ? (type >= 4 ? 1 : 3) : 20);
    for (i = 1; i <= n; ++i) {
        int sz = i % 10 == 0 ? SPR_SCALE * 3 :
            i % 3 == 0 ? SPR_SCALE * 2 : SPR_SCALE;
        int r = 0, g = 0, b = 0;
        if (type % 2 == 0) {
            int col = i % 3 == 0 ? 32 : 16;
            r = 255 - rand() % col;
            g = 255 - rand() % col;
            b = 255 - rand() % col;
            switch (rand() % 3) {
                case 0: r = 255; break;
                case 1: g = 255; break;
                case 2: b = 255; break;
            }
        } else {
            r = 160 - rand() % 16;
            g = 240;
            b = 112 - rand() % 16;
        }
        particle_add(&this->particle,
            this->simulator->prot.x * UNIT_PX + rand() % 31 - 15,
            this->simulator->prot.y * UNIT_PX + rand() % 31 - 15,
            -this->simulator->prot.vx * UNIT_PX / 3 + rand() % 31 - 15,
            UNIT_PX * 3 + rand() % 31 - 15,
            sz, sz,
            type >= 2 ? 0.15 : 0.25,
            type >= 2 ? 0.3 : 1, r, g, b);
    }
}

static inline void switch_stage_ctx(gameplay_scene *this)
{
    /* Update player's record */
    if (this->prev_sim != NULL) {
        profile_stage *ps = profile_get_stage(this->chap->idx, this->cur_stage_idx);
        int cmbid = modcomb_id(this->mods);
        int stage_time = (int)((this->simulator->cur_time - this->stage_start_time) * 48);
        if (ps->time[cmbid] == -1 || ps->time[cmbid] > stage_time) {
            ps->time[cmbid] = stage_time;
            ps->cleared = true;
            profile_save();
        }
    }

    /* Go to the next stage or the chapter finish accordingly */
    if (++this->cur_stage_idx == this->chap->n_stages) {
        this->disp_state = DISP_CHAPFIN;
        this->total_time = this->simulator->cur_time;
    } else {
        this->rec = this->chap->stages[this->cur_stage_idx];
        this->simulator = stage_create_sim(this->rec);
        this->stage_start_time =
        this->simulator->cur_time = (this->prev_sim == NULL ?
            get_audio_position(this) / this->chap->beat_mul :
            this->prev_sim->cur_time);
        /* Update hints */
        int i, j;
        for (i = 0; i < this->rec->hint_ct; ++i) {
            if (this->rec->hints[i].str != NULL)
                label_set_keyed_text(this->l_hints[i],
                    this->rec->hints[i].str, this->rec->hints[i].key);
            int w;
            int sig = this->chap->sig * this->rec->hints[i].mul;
            for (j = 0; j < sig; ++j) if (this->rec->hints[i].img != NULL) {
                sprite_reload(this->s_hints[i][j], this->rec->hints[i].img);
                if (j == 0) w = this->s_hints[i][j]->_base.dim.w / sig;
                /* The image should be horizontally sliced into `sig` pieces */
                this->s_hints[i][j]->tex.range.x = w * j;
                this->s_hints[i][j]->tex.range.w = w;
                this->s_hints[i][j]->_base.dim.w = w;
            }
            this->w_hints[i] = w;
        }
    }
}

static inline void get_camera_delta(gameplay_scene *this, double *dx, double *dy)
{
    double dest_x = clamp(this->simulator->prot.x,
        this->rec->cam_c1 + WIN_W_UNITS / 2, this->rec->cam_c2 - WIN_W_UNITS / 2);
    double dest_y = clamp(this->simulator->prot.y,
        this->rec->cam_r1 + WIN_H_UNITS / 2, this->rec->cam_r2 - WIN_H_UNITS / 2);
    *dx = dest_x - (this->cam_x + WIN_W_UNITS / 2);
    *dy = dest_y - (this->cam_y + WIN_H_UNITS / 2);
}

static inline void update_camera(gameplay_scene *this, double rate)
{
    double cam_dx, cam_dy;
    get_camera_delta(this, &cam_dx, &cam_dy);
    this->cam_x += rate * cam_dx;
    this->cam_y += rate * cam_dy;
}

static inline bool is_camera_fast(gameplay_scene *this)
{
    double cam_dx, cam_dy;
    get_camera_delta(this, &cam_dx, &cam_dy);
    return (sqr(cam_dx) + sqr(cam_dy) >= 1);
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
    this->dialogue_triggered = 0;
    update_camera(this, 1);
    this->retry_count++;
}

static void gameplay_scene_tick(gameplay_scene *this, double dt)
{
    if (this->paused) return;
    if (this->disp_state == DISP_LEADIN) {
        if ((this->disp_time -= dt) <= 0) {
            this->disp_state = DISP_NORMAL;
            SDL_DestroyTexture(this->leadin_tex);
            this->leadin_tex = NULL;
        }
    } else if (this->disp_state == DISP_FAILURE) {
        if ((this->disp_time -= dt) <= 0) {
            /* Run a transition to reset the stage */
            if (g_stage == (scene *)this)
                g_stage = (scene *)utransition_fade_create(
                    &g_stage, 1, (utransition_callback)retry_reinit);
        }
        this->simulator->cur_time += dt / (BEAT * this->chap->beat_mul);
        return;
    } else if (this->disp_state == DISP_DIALOGUE_IN) {
        if (g_stage == (scene *)this && this->dialogue_idx == -1) {
            this->disp_state = DISP_DIALOGUE_OUT;
            this->disp_time = DIALOGUE_ZOOM_DUR;
        } else {
            double r = (this->disp_time -= dt);
            if (r < 0 && g_stage == (scene *)this) {
                stage_dialogue *d = bekter_at_ptr(this->rec->plot,
                    this->dialogue_idx, stage_dialogue);
                g_stage = (scene *)dialogue_create(&g_stage, d->content);
                this->dialogue_idx = -1;
            }
            r = (r < 0) ? 1 : 1 - r / DIALOGUE_ZOOM_DUR;
            r = ease_quad_inout(r);
            r = 1 + (DIALOGUE_ZOOM_SCALE - 1) * r;
            this->scale = r;
        }
    } else if (this->disp_state == DISP_DIALOGUE_OUT) {
        if ((this->disp_time -= dt) <= 0) {
            this->disp_state = DISP_NORMAL;
            this->scale = 1;
        } else {
            double r = (this->disp_time -= dt);
            r = (r < 0) ? 0 : r / DIALOGUE_ZOOM_DUR;
            r = ease_quad_inout(r);
            r = 1 + (DIALOGUE_ZOOM_SCALE - 1) * r;
            this->scale = r;
        }
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
            if (this->disp_state != DISP_NORMAL) break;
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
            this->refill_time = REFILL_PERSISTENCE;
            this->simulator->prot.tag = 0;
            break;
    }

    this->simulator->prot.ay =
        (this->ver_state == VER_STATE_DOWN) ? 4.0 * SIM_GRAVITY : 0;
    double plunge_vy = this->simulator->prot.ay;
    double hor_mov_vx =
        (this->hor_state == HOR_STATE_LEFT) ? -HOR_SPD :
        (this->hor_state == HOR_STATE_RIGHT) ? +HOR_SPD : 0;
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
        /* Cancel out the vertical velocity */
        this->simulator->prot.vx = -hor_mov_vx / 2;
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
                this->simulator->prot.ay = -SIM_GRAVITY - 0.1;
                if ((this->mov_state & 3) != 3) this->simulator->prot.vy = 0;
            }
            if (this->mov_state & MOV_VERDASH & 3) {
                /* If a diagonal dash is taking place,
                 * the vertical acceleration is overridden here */
                this->simulator->prot.ay =
                    (this->mov_state & MOV_DASH_UP) ? DASH_VER_ACCEL : 0;
            }
            if ((this->mov_state & 3) == 3) {
                this->simulator->prot.ax *= DASH_DIAG_SCALE;
                this->simulator->prot.ay *= DASH_DIAG_SCALE;
            }
            /* In case the protagonist runs into something,
             * the velocity becomes 0 and should not change any more */
            if (this->simulator->prot.vx * this->simulator->prot.ax > 1e-8)
                this->simulator->prot.ax = 0;
            if (this->simulator->prot.vy * this->simulator->prot.ay > 1e-8)
                this->simulator->prot.ay = 0;
            /* In case a plunge is in progress, its speed change
             * should be taken into account */
            this->simulator->prot.ay += plunge_vy;
        }
        this->mov_time -= dt / BEAT;
        add_dash_particles(this,
            (this->mov_time > DASH_DUR / 2 ? 2 : 4) |
            ((this->mov_state & MOV_USING_REFILL) ? 1 : 0));
    } else {
        /* Normal state */
        this->simulator->prot.vx = 0;
    }
    this->simulator->prot.vx += hor_mov_vx;

    double rt = this->rem_time + dt / (BEAT * this->chap->beat_mul);
    while (rt >= SIM_STEPLEN) {
        sim_tick(this->simulator);
        rt -= SIM_STEPLEN;
    }
    this->rem_time = rt;

    if (this->refill_time >= 0)
        this->refill_time -= dt / (BEAT * this->chap->beat_mul);

    particle_tick(&this->particle, dt);

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
            this->disp_state = DISP_DIALOGUE_IN;
            this->disp_time = DIALOGUE_ZOOM_DUR;
            this->dialogue_idx = i;
            break;
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

static inline int align_pixel(double x)
{
    return iround(x / SPR_SCALE) * SPR_SCALE;
}

static inline void render_object(gameplay_scene *this,
    bool rounds, int cxi, int cyi, int sx, int sy, sobj *o)
{
    int x = align_pixel(((rounds ? (int)o->x : o->x) + o->tx) * UNIT_PX) - cxi,
        y = align_pixel(((rounds ? (int)o->y : o->y) + o->ty) * UNIT_PX) - cyi;
    if (this->scale == 1) {
        render_texture_scaled(get_texture(this, o), x, y, SPR_SCALE);
    } else {
        x = round(sx + (x - sx) * this->scale);
        y = round(sy + (y - sy) * this->scale);
        render_texture_scaled(get_texture(this, o),
            x, y, SPR_SCALE * this->scale);
        render_texture_scaled(get_texture(this, o),
            x + 1, y + 1, SPR_SCALE * this->scale);
    }
}

static inline void render_objects(gameplay_scene *this,
    bool is_prev, bool is_after, double offsx, double offsy,
    int scale_x, int scale_y)
{
    sim *sim = (is_prev ? this->prev_sim : this->simulator);
    double cx = (is_prev ? this->cam_x + offsx : this->cam_x);
    double cy = (is_prev ? this->cam_y + offsy : this->cam_y);
    int rmin = clamp(floorf(cy), 0, sim->grows),
        rmax = clamp(ceilf(cy + WIN_H_UNITS), 0, sim->grows),
        cmin = clamp(floorf(cx), 0, sim->gcols),
        cmax = clamp(ceilf(cx + WIN_W_UNITS), 0, sim->gcols);
    int r, c;
    int cxi = align_pixel(cx * UNIT_PX),
        cyi = align_pixel(cy * UNIT_PX);
    for (r = rmin; r < rmax; ++r)
        for (c = cmin; c < cmax; ++c) {
            sobj *o = &sim_grid(sim, r, c);
            if (o->tag != 0 && ((o->tag < OBJID_DRAW_AFTER) ^ is_after))
                render_object(this, true, cxi, cyi, scale_x, scale_y, o);
        }
    for (r = 0; r < sim->anim_sz; ++r) {
        sobj *o = sim->anim[r];
        if ((o->tag < OBJID_DRAW_AFTER) ^ is_after)
            render_object(this, false, cxi, cyi, scale_x, scale_y, o);
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

static inline bool get_pos_in_bar(gameplay_scene *this, double ant, int mul,
    int *beats_i, double *beats_d)
{
    double beats = get_audio_position(this) * mul;
    if (this->disp_state == DISP_LEADIN) return false;
    if (this->mods & MOD_SOTTO_VOCO) return false;
    if (beats < -ant) return false;
    *beats_i = (int)(beats + ant);
    *beats_d = beats - *beats_i;
    *beats_i %= (this->chap->sig * mul);
    *beats_d /= this->chap->beat_mul;
    return true;
}

static inline void draw_overlay(gameplay_scene *this)
{
    int beats_i;
    double beats_d;
    if (!get_pos_in_bar(this, 1./24, 1, &beats_i, &beats_d)) return;
    bool is_downbeat = this->chap->dash_mask & (1 << beats_i),
        is_upbeat = this->chap->hop_mask & (1 << beats_i);
    if (!is_upbeat) return;
    int opacity = iround((
        beats_d < 0 ? (1 + beats_d * 24) :
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
                int a = iround(255 * (r - rsqr) / (rosqr - rsqr));
                *((int *)(pixdata + j * pitch) + i) = a;
            } else {
                *((int *)(pixdata + j * pitch) + i) = 255;
            }
        }
    SDL_UnlockTexture(tex);
}

static inline void pause_sound(gameplay_scene *this)
{
    this->paused = true;
    int i;
    for (i = 0; i < this->chap->n_tracks; ++i)
        orion_pause(&g_orion, TRACKID_STAGE_BGM + i);
}

static inline void update_sound(gameplay_scene *this)
{
    int i;
    if ((scene *)this == g_stage) {
        this->paused = false;
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
        for (i = 0; i < this->chap->n_tracks; ++i) {
#ifndef NDEBUG
            printf("%.4lf%c", val[i] / sum, i == this->chap->n_tracks - 1 ? '\n' : ' ');
#endif
            orion_ramp(&g_orion, TRACKID_STAGE_BGM + i,
                0.03, val[i] / sum * profile.bgm_vol * VOL_VALUE);
        }
    }
}

/* XXX: This is too long, hopefully it can be split into smaller methods */
static void gameplay_scene_draw(gameplay_scene *this)
{
    update_sound(this);

    SDL_SetRenderDrawColor(g_renderer, 216, 224, 255, 255);
    SDL_RenderClear(g_renderer);

    int cxi = align_pixel(this->cam_x * UNIT_PX),
        cyi = align_pixel(this->cam_y * UNIT_PX);
    bool is_prot_fast =
        sqr(this->simulator->prot.vx) + sqr(this->simulator->prot.vy)
        >= HOR_SPD * HOR_SPD;
    double prot_disp_x = this->simulator->prot.x * UNIT_PX;
    double prot_disp_y = this->simulator->prot.y * UNIT_PX;
    double prot_w = this->simulator->prot.w * UNIT_PX;
    double prot_h = this->simulator->prot.h * UNIT_PX;

    if (!is_prot_fast) {
        prot_disp_x = align_pixel(prot_disp_x) - cxi;
        prot_disp_y = align_pixel(prot_disp_y) - cyi;
    } else {
        prot_disp_x -= iround(this->cam_x * UNIT_PX);
        prot_disp_y -= iround(this->cam_y * UNIT_PX);
    }

    /* Draw the previous stage during transition */
    if (this->prev_sim != NULL) {
        int delta_x = this->simulator->worldc - this->prev_sim->worldc;
        int delta_y = this->simulator->worldr - this->prev_sim->worldr;
        render_objects(this, true, false, delta_x, delta_y, prot_disp_x, prot_disp_y);
        render_objects(this, true, true, delta_x, delta_y, prot_disp_x, prot_disp_y);
    }

    render_objects(this, false, false, 0, 0, prot_disp_x, prot_disp_y);

    texture prot_tex = this->rec->prot_tex;
    if (this->disp_state == DISP_FAILURE) {
        int f_idx = clamp(FAILURE_NF - (int)(this->disp_time / FAILURE_SPF) - 1,
            0, FAILURE_NF - 1);
        prot_tex = this->rec->prot_fail_tex[f_idx];
        /* The failure animation should be displayed above everything else;
         * No dialogue or stage transition
         * should be running during failure animation */
        render_objects(this, false, true, 0, 0, 0, 0);
        prot_disp_x -= (prot_tex.range.w * SPR_SCALE - prot_w) / 2;
        prot_disp_y -= (prot_tex.range.h * SPR_SCALE - prot_h) / 2;
        prot_w = prot_tex.range.w * SPR_SCALE;
        prot_h = prot_tex.range.h * SPR_SCALE;
    }

    render_texture_ex(prot_tex, &(SDL_Rect){
        prot_disp_x, prot_disp_y,
        iround(prot_w * this->scale), iround(prot_h * this->scale),
    }, 0, NULL, (this->facing == HOR_STATE_LEFT ? SDL_FLIP_HORIZONTAL : 0));

    if (this->disp_state != DISP_FAILURE)
        render_objects(this, false, true, 0, 0, prot_disp_x, prot_disp_y);

    /* Display hints */
    int i, j;
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 128);
    for (i = 0; i < this->rec->hint_ct; ++i) {
        int x, y, w, h;
        x = align_pixel((this->rec->hints[i].c + 0.5) * UNIT_PX) - cxi;
        y = align_pixel((this->rec->hints[i].r + 0.5) * UNIT_PX) - cyi;
        if (this->rec->hints[i].str != NULL) {
            element_place_anchored((element *)this->l_hints[i], x, y, 0.5, 0.5);
            x = this->l_hints[i]->_base._base.dim.x - HINT_PADDING;
            y = this->l_hints[i]->_base._base.dim.y - HINT_PADDING;
            w = this->l_hints[i]->_base._base.dim.w + HINT_PADDING * 2;
            h = this->l_hints[i]->_base._base.dim.h + HINT_PADDING * 2;
        } else {
            w = h = 0;
        }
        int sig;
        if (this->rec->hints[i].img != NULL) {
            /* Expand horizontally if necessary */
            sig = this->chap->sig * this->rec->hints[i].mul;
            int w1 = this->w_hints[i] * sig + HINT_PADDING * 2;
            int xoff = 0;   /* Offset of sprites, in order to centre-align */
            if (w < w1) {
                x -= (w1 - w) / 2;
                w = w1;
            } else {
                xoff = (w - w1) / 2;
            }
            /* Display all sprites */
            for (j = 0; j < sig; ++j) {
                element_place((element *)this->s_hints[i][j],
                    x + HINT_PADDING + this->w_hints[i] * j + xoff,
                    y + h - HINT_PADDING);
            }
            /* Update height */
            h += this->s_hints[i][0]->_base.dim.h - HINT_PADDING;
            if (this->rec->hints[i].str == NULL) {
                y -= HINT_PADDING;
                h += HINT_PADDING;
            }
        }
        SDL_RenderFillRect(g_renderer, &(SDL_Rect){x, y, w, h});
        element_draw((element *)this->l_hints[i]);
        /* Display images */
        if (this->rec->hints[i].img != NULL) {
            int beats_i;
            double beats_d;
            bool started = get_pos_in_bar(this, 0,
                this->rec->hints[i].mul, &beats_i, &beats_d);
            for (j = 0; j < sig; ++j) {
                double prog = (started && beats_i == j ? 1 - beats_d : 0);
                if (this->rec->hints[i].mask & (1 << beats_i)) {
                    this->s_hints[i][j]->alpha = iround(96 + 159 * prog);
                    SDL_SetTextureColorMod(this->s_hints[i][j]->tex.sdl_tex,
                        255, 255, iround((1 - prog) * 255));
                } else {
                    this->s_hints[i][j]->alpha = iround(96 * (1 - prog));
                }
                element_draw((element *)this->s_hints[i][j]);
            }
        }
    }

    /* Lead-in or modifier flashlight */
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
            iround(WIN_W - prot_disp_x), iround(WIN_H - prot_disp_y),
            WIN_W, WIN_H
        }, NULL);
    }

    /* Draw particles */
    particle_draw_aligned(&this->particle, -cxi, -cyi, SPR_SCALE);

    draw_overlay(this);

    /* The clock should not be disturbed by the metronome */
    if (profile.show_clock) {
        int sig = this->chap->sig;
        double r = this->simulator->cur_time - this->stage_start_time;
        int chap_time = (int)(this->simulator->cur_time * 48);
        int stage_time = (int)(r * 48);
        if (stage_time < -0.1) return;
        char s[16], d[8];

        chap_time *= this->chap->beat_mul;
        stage_time *= this->chap->beat_mul;
        r = (r < CLOCK_BLINK_DUR) ? r / CLOCK_BLINK_DUR : 1;

        /* Overall clock */
        sprintf(s, "%03d. %02d. ", chap_time / (sig * 48), chap_time % (sig * 48) / 48);
        sprintf(d, "%02d", chap_time % 48);
        label_set_text(this->clock_chap, s);
        label_set_text(this->clock_chap_dec, d);
        element_place_anchored((element *)this->clock_chap, 24, 48, 0, 0.5);
        element_place((element *)this->clock_chap_dec,
            this->clock_chap->_base._base.dim.x + this->clock_chap->_base._base.dim.w,
            this->clock_chap->_base._base.dim.y);
        if (r < 1 && this->cur_stage_idx == this->start_stage_idx) {
            int dx = (1 - ease_elastic_out(r + 0.1, 3)) * (WIN_W / 4);
            this->clock_chap->_base._base.dim.x -= dx;
            this->clock_chap_dec->_base._base.dim.x -= dx;
        }
        element_draw((element *)this->clock_chap);
        element_draw((element *)this->clock_chap_dec);

        /* Stage clock */
        sprintf(s, "%03d. %02d. ", stage_time / (sig * 48), stage_time % (sig * 48) / 48);
        sprintf(d, "%02d", stage_time % 48);
        label_set_text(this->clock_stg, s);
        label_set_text(this->clock_stg_dec, d);
        element_place_anchored((element *)this->clock_stg, 24, 96, 0, 0.5);
        element_place((element *)this->clock_stg_dec,
            this->clock_stg->_base._base.dim.x + this->clock_stg->_base._base.dim.w,
            this->clock_stg->_base._base.dim.y);
        if (r < 1) {
            if (this->cur_stage_idx == this->start_stage_idx) {
                int dx = (1 - ease_elastic_out(r, 3)) * (WIN_W / 4);
                this->clock_stg->_base._base.dim.x -= dx;
                this->clock_stg_dec->_base._base.dim.x -= dx;
                SDL_SetTextureColorMod(this->clock_stg->_base.tex.sdl_tex, 160, 160, 160);
                SDL_SetTextureColorMod(this->clock_stg_dec->_base.tex.sdl_tex, 160, 160, 160);
            } else {
                int lr = lerp(r, 255, 160),
                    lg = lerp(r, 216, 160),
                    lb = lerp(r, 128, 160);
                SDL_SetTextureColorMod(this->clock_stg->_base.tex.sdl_tex, lr, lg, lb);
                SDL_SetTextureColorMod(this->clock_stg_dec->_base.tex.sdl_tex, lr, lg, lb);
            }
        } else {
            SDL_SetTextureColorMod(this->clock_stg->_base.tex.sdl_tex, 160, 160, 160);
            SDL_SetTextureColorMod(this->clock_stg_dec->_base.tex.sdl_tex, 160, 160, 160);
        }
        element_draw((element *)this->clock_stg);
        element_draw((element *)this->clock_stg_dec);
    }
}

static void gameplay_scene_drop(gameplay_scene *this)
{
    if (this->leadin_tex != NULL) SDL_DestroyTexture(this->leadin_tex);
    if (this->prev_sim != NULL) sim_drop(this->prev_sim);
    if (this->simulator != NULL && this->simulator != this->prev_sim)
        sim_drop(this->simulator);
    pause_sound(this);
    int i, j;
    for (i = 0; i < MAX_HINTS; ++i) {
        element_drop(this->l_hints[i]);
        for (j = 0; j < MAX_SIG; ++j)
            element_drop(this->s_hints[i][j]);
    }
    if (profile.show_clock) {
        element_drop(this->clock_chap);
        element_drop(this->clock_chap_dec);
        element_drop(this->clock_stg);
        element_drop(this->clock_stg_dec);
    }
}

static void try_hop(gameplay_scene *this)
{
    char t;
    if ((t = cant_hop(this)) != 0) {
        add_hop_particles(this, t);
        return;
    }
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
    } else return;
    add_hop_particles(this, 0);
}

static void try_dash(gameplay_scene *this, bool is_dirchg)
{
    char t;
    if ((t = cant_dash(this, is_dirchg)) != 0 && t != 4) {
        if (!is_dirchg) add_hop_particles(this, t);
        return;
    }
    /* In case of multiple dashes in one beat, simply exit without particles */
    if (!is_dirchg && (this->mov_state & MOV_DASH_BASE) && t != 4) return;
    /* In case of direction updates, the time should not be reset */
    double dur = is_dirchg ? this->mov_time : DASH_DUR;
    if (is_dirchg && dur < DASH_MIN_DUR) return;
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
    this->mov_state =
        MOV_DASH_BASE | dir_has | dir_denotes | (t == 4 ? MOV_USING_REFILL : 0);
    this->mov_time = dur;
    if (!is_dirchg) add_dash_particles(this, t == 4 ? 1 : 0);
}

static void gameplay_scene_key_handler(gameplay_scene *this, SDL_KeyboardEvent *ev)
{
#define toggle(__thisstate, __keystate, __has, __none) do { \
    if ((__keystate) == SDL_PRESSED) (__thisstate) = (__has); \
    else if ((__thisstate) == (__has)) (__thisstate) = (__none); \
} while (0)

    if (this->disp_state != DISP_NORMAL) return;
    if (ev->keysym.sym != SDLK_ESCAPE && get_audio_position(this) < 0) return;
    switch (ev->keysym.sym) {
        case SDLK_c:
            if (!ev->repeat && ev->state == SDL_PRESSED) try_hop(this);
            break;
        case SDLK_x:
            if (!ev->repeat && ev->state == SDL_PRESSED) try_dash(this, false);
            break;
        case SDLK_UP:
            toggle(this->ver_state, ev->state, VER_STATE_UP, VER_STATE_NONE);
            if (ev->state == SDL_PRESSED && (this->mov_state & MOV_DASH_BASE))
                try_dash(this, true);
            break;
        case SDLK_DOWN:
            toggle(this->ver_state, ev->state, VER_STATE_DOWN, VER_STATE_NONE);
            if (ev->state == SDL_PRESSED && (this->mov_state & MOV_DASH_BASE))
                try_dash(this, true);
            break;
        case SDLK_LEFT:
            if (ev->state == SDL_PRESSED) this->facing = HOR_STATE_LEFT;
            toggle(this->hor_state, ev->state, HOR_STATE_LEFT, HOR_STATE_NONE);
            if (ev->state == SDL_PRESSED && (this->mov_state & MOV_DASH_BASE))
                try_dash(this, true);
            break;
        case SDLK_RIGHT:
            if (ev->state == SDL_PRESSED) this->facing = HOR_STATE_RIGHT;
            toggle(this->hor_state, ev->state, HOR_STATE_RIGHT, HOR_STATE_NONE);
            if (ev->state == SDL_PRESSED && (this->mov_state & MOV_DASH_BASE))
                try_dash(this, true);
            break;
        case SDLK_ESCAPE:
            if (ev->state == SDL_PRESSED && g_stage == (scene *)this) {
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
    ret->_base.tick = (scene_tick_func)gameplay_scene_tick;
    ret->_base.draw = (scene_draw_func)gameplay_scene_draw;
    ret->_base.drop = (scene_drop_func)gameplay_scene_drop;
    ret->_base.key_handler = (scene_key_func)gameplay_scene_key_handler;
    ret->bg = bg;

    ret->rem_time = 0;
    ret->facing = HOR_STATE_RIGHT;

    if (mods & MOD_STRETTO) mods |= MOD_SEMPLICE;
    ret->mods = mods;
    if (mods & MOD_VIVACE) ret->mul = VIVACE_MUL;
    else if (mods & MOD_ANDANTE) ret->mul = ANDANTE_MUL;
    else ret->mul = 1;

    /* Sound should be loaded before the stage, as
     * the play position will be used to initialize the simulator */
    int i;
    for (i = 0; i < chap->n_tracks; ++i) {
        if (chap->tracks[i].src_id == -1) {
            orion_load_ogg(&g_orion, TRACKID_STAGE_BGM + i, chap->tracks[i].str);
            if (ret->mul != 1)
                orion_apply_stretch(&g_orion,
                    TRACKID_STAGE_BGM + i, TRACKID_STAGE_BGM + i,
                    (ret->mul - 1) * 100);
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
            (int)(chap->offs / ret->mul * 44100),
            (int)((chap->offs + chap->beat * chap->loop) / ret->mul * 44100));
        orion_ramp(&g_orion, TRACKID_STAGE_BGM + i, 0, 0);
        orion_pause(&g_orion, TRACKID_STAGE_BGM + i);
    }

    int j;
    for (i = 0; i < MAX_HINTS; ++i) {
        ret->l_hints[i] = label_create(FONT_UPRIGHT, HINT_FONTSZ,
            (SDL_Color){255, 255, 255}, WIN_W, "");
        for (j = 0; j < MAX_SIG; ++j)
            ret->s_hints[i][j] = sprite_create_empty();
    }

    if (profile.show_clock) {
        label *l = label_create_outlined(FONT_UPRIGHT, FONT_UPRIGHT_OUTLINE, CLOCK_CHAP_FONTSZ,
            (SDL_Color){255, 255, 255}, (SDL_Color){64, 64, 64}, WIN_W, "");
        ret->clock_chap = l;
        l = label_create_outlined(FONT_UPRIGHT, FONT_UPRIGHT_OUTLINE, CLOCK_CHAP_FONTSZ * 3 / 4,
            (SDL_Color){255, 255, 255}, (SDL_Color){64, 64, 64}, WIN_W, "");
        ret->clock_chap_dec = l;
        l = label_create_outlined(FONT_UPRIGHT, FONT_UPRIGHT_OUTLINE, CLOCK_STG_FONTSZ,
            (SDL_Color){255, 255, 255}, (SDL_Color){64, 64, 64}, WIN_W, "");
        ret->clock_stg = l;
        l = label_create_outlined(FONT_UPRIGHT, FONT_UPRIGHT_OUTLINE, CLOCK_STG_FONTSZ * 3 / 4,
            (SDL_Color){255, 255, 255}, (SDL_Color){64, 64, 64}, WIN_W, "");
        ret->clock_stg_dec = l;
    }

    ret->prev_sim = NULL;
    ret->chap = chap;
    ret->cur_stage_idx = idx - 1;
    switch_stage_ctx(ret);
    ret->start_stage_idx = idx;
    ret->stage_start_time = 0;
    ret->refill_time = -1;

    particle_init(&ret->particle);

    ret->cam_x = clamp(ret->simulator->prot.x,
        WIN_W_UNITS / 2, ret->simulator->gcols - WIN_W_UNITS / 2) - WIN_W_UNITS / 2;
    ret->cam_y = clamp(ret->simulator->prot.y,
        WIN_H_UNITS / 2, ret->simulator->grows - WIN_H_UNITS / 2) - WIN_H_UNITS / 2;
    ret->scale = 1;

    return ret;
}
