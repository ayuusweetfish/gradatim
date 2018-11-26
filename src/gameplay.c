#include "gameplay.h"

#include "global.h"
#include "bekter.h"
#include "stage_rec.h"
#include "unary_transition.h"
#include "pause.h"

#include <math.h>

static const double UNIT_PX = 48;
static const double WIN_W_UNITS = (double)WIN_W / UNIT_PX;
static const double WIN_H_UNITS = (double)WIN_H / UNIT_PX;
static const double SPR_SCALE = 3;

static const double AUD_OFFSET = +0.04;
static const double BEAT = 60.0 / 128;  /* Temporary */
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

static const double LEADIN_INIT = 1.2;
static const double LEADIN_DUR = 0.5; /* Seconds */
static const double FAILURE_SPF = 0.1;

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

static inline double get_audio_position()
{
    double sec = (double)orion_tell(&g_orion, TRACKID_STAGE_BGM) / 44100;
    return (sec + AUD_OFFSET) / BEAT;
}

static inline void load_csv(gameplay_scene *this, const char *path)
{
    struct stage_rec *rec = stage_read(path);
    if (!rec) return;   /* XXX: Uneffective error handling */
    if (this->rec) stage_drop(this->rec);
    this->simulator = rec->sim;
    this->rec = rec;
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

static void retry_reinit(gameplay_scene *this)
{
    this->hor_state = HOR_STATE_NONE;
    this->facing = HOR_STATE_RIGHT;
    this->ver_state = VER_STATE_NONE;
    this->simulator->prot.tag = 0;
    this->simulator->prot.x = this->rec->spawn_c;
    this->simulator->prot.y = this->rec->spawn_r;
    this->disp_state = DISP_NORMAL;
    sim_reinit(this->simulator);
    this->simulator->cur_time = get_audio_position() - this->aud_sim_offset;
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
            this->simulator->cur_time = get_audio_position() - this->aud_sim_offset;
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
                load_csv(this, "rub.csv");
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

    double rt = this->rem_time + dt / BEAT;
    while (rt >= SIM_STEPLEN) {
        sim_tick(this->simulator);
        rt -= SIM_STEPLEN;
    }
    this->rem_time = rt;

    /* Horizontal speed should be cancelled out immediately
     * However, not if such will result in a 'bounce' */
    if (this->simulator->prot.vx * (this->simulator->prot.vx - hor_mov_vx) > 0)
        this->simulator->prot.vx -= hor_mov_vx;

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
    for (r = rmin; r < rmax; ++r)
        for (c = cmin; c < cmax; ++c) {
            sobj *o = &sim_grid(sim, r, c);
            if (o->tag != 0) {
                render_texture_scaled(get_texture(this, o),
                    ((int)o->x - cx) * UNIT_PX,
                    ((int)o->y - cy) * UNIT_PX,
                    SPR_SCALE
                );
            }
        }
    for (r = 0; r < sim->anim_sz; ++r) {
        sobj *o = sim->anim[r];
        render_texture_scaled(get_texture(this, o),
            (o->x + o->tx - cx) * UNIT_PX,
            (o->y + o->ty - cy) * UNIT_PX,
            SPR_SCALE
        );
    }
}

static inline void run_leadin(gameplay_scene *this)
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
    double beats = get_audio_position();
    if (this->disp_state == DISP_LEADIN) {
        this->aud_sim_offset = beats - this->simulator->cur_time;
        this->aud_sim_offset_n_samples++;
    }
    int beats_i = (int)(beats + 1./16);
    double beats_d = beats - beats_i;
    double is_downbeat = (beats_i % 4 == 0);
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

static void gameplay_scene_draw(gameplay_scene *this)
{
    if ((scene *)this == g_stage) orion_resume(&g_orion, TRACKID_STAGE_BGM);

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

    if (this->disp_state == DISP_LEADIN) {
        prot_disp_x += this->simulator->prot.w / 2 * UNIT_PX;
        prot_disp_y += this->simulator->prot.h / 2 * UNIT_PX;
        double radius = 0, radius_o = 0;
        if (this->disp_time <= LEADIN_DUR) {
            radius = UNIT_PX + WIN_W * (1 - this->disp_time / LEADIN_DUR);
            radius_o = radius + UNIT_PX * 0.5;
        } else {
            double t = this->disp_time - LEADIN_DUR;
            double p = ease_elastic_out(1 - t / LEADIN_INIT, 0.3);
            radius_o = p * UNIT_PX * 1.5;
            radius = radius_o - UNIT_PX * 0.5;
            if (radius < 0) radius = 0;
        }
        double radius_sqr = sqr(radius);
        double radius_o_sqr = sqr(radius_o);
        int i, j;
        void *pixdata;
        int pitch;
        SDL_LockTexture(this->leadin_tex, NULL, &pixdata, &pitch);
        for (j = 0; j < WIN_H; ++j)
            for (i = 0; i < WIN_W; ++i) {
                double r = sqr(i - prot_disp_x) + sqr(j - prot_disp_y);
                if (r <= radius_sqr) {
                    *((int *)(pixdata + j * pitch) + i) = 0;
                } else if (r <= radius_o_sqr) {
                    int a = round(255 * (r - radius_sqr) / (radius_o_sqr - radius_sqr));
                    *((int *)(pixdata + j * pitch) + i) = a;
                } else {
                    *((int *)(pixdata + j * pitch) + i) = 255;
                }
            }
        SDL_UnlockTexture(this->leadin_tex);
        SDL_RenderCopy(g_renderer, this->leadin_tex, NULL, NULL);
    }

    draw_overlay(this);
}

static void gameplay_scene_drop(gameplay_scene *this)
{
    sim_drop(this->simulator);
    if (this->prev_sim) sim_drop(this->prev_sim);
    stage_drop(this->rec);
    orion_pause(&g_orion, TRACKID_STAGE_BGM);
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
                g_stage = (scene *)pause_scene_create(&g_stage, this->bg);
                orion_pause(&g_orion, TRACKID_STAGE_BGM);
            }
            break;
        default: break;
    }
}

gameplay_scene *gameplay_scene_create(scene **bg)
{
    gameplay_scene *ret = malloc(sizeof(gameplay_scene));
    memset(ret, 0, sizeof(gameplay_scene));
    ret->_base.children = bekter_create();
    ret->_base.tick = (scene_tick_func)gameplay_scene_tick;
    ret->_base.draw = (scene_draw_func)gameplay_scene_draw;
    ret->_base.drop = (scene_drop_func)gameplay_scene_drop;
    ret->_base.key_handler = (scene_key_func)gameplay_scene_key_handler;
    ret->bg = *bg;
    ret->bg_ptr = bg;

    run_leadin(ret);

    ret->rem_time = 0;
    ret->facing = HOR_STATE_RIGHT;

    ret->prev_sim = NULL;
    load_csv(ret, "rua.csv");
    ret->cam_x = clamp(ret->simulator->prot.x,
        WIN_W_UNITS / 2, ret->simulator->gcols - WIN_W_UNITS / 2) - WIN_W_UNITS / 2;
    ret->cam_y = clamp(ret->simulator->prot.y,
        WIN_H_UNITS / 2, ret->simulator->grows - WIN_H_UNITS / 2) - WIN_H_UNITS / 2;

    orion_load_ogg(&g_orion, TRACKID_STAGE_BGM, "4-5.ogg");
    orion_play_loop(&g_orion, TRACKID_STAGE_BGM, 0, 0, -1);

    return ret;
}
