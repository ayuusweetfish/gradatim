#include "sobj.h"
#include "sim.h"
#include "../resources.h"

#include <math.h>
#include <stdbool.h>

#define max(__a, __b) ((__a) > (__b) ? (__a) : (__b))
#define take_max(__var, __val) if ((__var) < (__val)) (__var) = (__val)

static const int FRAGILE_FRAMES = OBJID_FRAGILE_EMPTY - OBJID_FRAGILE - 1;
static const double FRAGILE_RECOVER_DUR = 4;
static const double SPRING_RECOVER_DUR = 1;
#define SPRING_SPD (1.5 * SIM_GRAVITY)

static inline bool is_landing(sobj *o, sobj *prot)
{
    return o->is_on && prot->y + prot->h <= o->y + o->h / 16;
}

static inline bool is_above(sobj *o, sobj *prot)
{
    return prot->y + prot->h <= o->y + o->h / 16;
}

static inline bool in_range(double x, double l, double h)
{
    return l <= x && x <= h;
}

static inline bool intsc_1d(double x1, double w1, double x2, double w2)
{
    return (x1 >= x2) ? (x2 + w2 >= x1) : (x1 + w1 >= x2);
}

static inline bool is_touching(sobj *o, sobj *prot, double w, double h)
{
    return intsc_1d(prot->x, prot->w, o->x, w) &&
        intsc_1d(prot->y, prot->h, o->y, h);
}

static inline bool is_intersecting(sobj *o, sobj *prot, double w, double h)
{
    return intsc_1d(prot->x + prot->w / 4, prot->w / 2, o->x, w) &&
        intsc_1d(prot->y + prot->w / 4, prot->h / 2, o->y, h);
}

static inline bool is_stuck(sobj *o, sobj *prot, double w, double h)
{
    return prot->x <= o->x + w / 2 && prot->x + prot->w >= o->x + w / 2 &&
        prot->y <= o->y + h / 2 && prot->y + prot->h >= o->y + h / 2;
}

static inline bool is_near(sobj *o, sobj *prot)
{
    static const double EPS = 1e-8;
    return (in_range(prot->x, o->x - EPS, o->x + o->w + EPS) ||
        in_range(prot->x + prot->w, o->x - EPS, o->x + o->w + EPS)) &&
        (in_range(prot->y, o->y - EPS, o->y + o->h + EPS) ||
        in_range(prot->y + prot->h, o->y - EPS, o->y + o->h + EPS));
}

static inline void update_offset(sobj *o)
{
    int tx, ty;
    grid_offset(o->tag, &tx, &ty);
    o->tx = -tx * 1./16;
    o->ty = -ty * 1./16;
}

static inline void bg_init(sobj *o)
{
    o->w = o->h = 0;
}

static const double TORCH_ANIMLEN = 2;
static const int TORCH_NFRAMES = OBJID_TORCH_LAST - OBJID_TORCH_FIRST;
static const double TORCH_FRAMEFRAC = TORCH_NFRAMES / TORCH_ANIMLEN;

static inline void torch_update_pred(sobj *o, double T, sobj *prot)
{
    o->tag = OBJID_TORCH_FIRST +
        (int)(fmod(T, TORCH_ANIMLEN) * TORCH_FRAMEFRAC);
}

static inline void fragile_update_pred(sobj *o, double T, sobj *prot)
{
    if (o->t != -1) {
        if (T - o->t < FRAGILE_FRAMES) {
            o->tag = OBJID_FRAGILE + (int)(T - o->t) + 1;
        } else if (T - o->t < FRAGILE_FRAMES + FRAGILE_RECOVER_DUR) {
            o->tag = OBJID_FRAGILE_EMPTY;
            o->w = o->h = 0;
        } else {
            o->tag = OBJID_FRAGILE;
            o->w = o->h = 1;
            o->t = -1;
        }
    }
}

static inline void fragile_update_post(sobj *o, double T, sobj *prot)
{
    if (o->tag == OBJID_FRAGILE && is_landing(o, prot)) o->t = T;
}

#define billow_sig(__o) ((int)(__o)->ay)
#define billow_beatmask(__o) ((int)(__o)->ax)
static const double BILLOW_ANIM = 0.25;
static const double BILLOW_FRMLEN =
    BILLOW_ANIM / (OBJID_BILLOW_EMPTY - OBJID_BILLOW - 1);

static inline void billow_init(sobj *o)
{
    o->tag = (billow_beatmask(o) & 1) ? OBJID_BILLOW : OBJID_BILLOW_EMPTY;
    o->t = -1;
}

static inline void billow_update_pred(sobj *o, double T, sobj *prot)
{
    int sig = billow_sig(o);
    int mask = billow_beatmask(o);
    int beat_i = (int)T % sig;
    double beat_d = T - (int)T;
    bool cur_beat = mask & (1 << beat_i);
    bool last_beat = mask & (1 << ((beat_i + sig - 1) % sig));
    bool next_beat = mask & (1 << ((beat_i + 1) % sig));

    /* Prevent getting the protagonist stuck inside
     * This is caused by the inaccuracy of event refreshes */
    if (o->t != -1 && is_intersecting(o, prot, 1, 1)) o->t = -1;

    if (o->tag != OBJID_BILLOW_EMPTY &&
        cur_beat && !next_beat && beat_d >= 1 - BILLOW_ANIM)
    {
        /* Starts disappearing */
        o->t = -1;
        int f_idx = (beat_d - 1 + BILLOW_ANIM) / BILLOW_FRMLEN;
        o->tag = OBJID_BILLOW + f_idx;
    } else if (o->t != -1) {
        int f_idx = (T - o->t) / BILLOW_FRMLEN;
        o->tag = max(OBJID_BILLOW, OBJID_BILLOW_EMPTY - 1 - f_idx);
    } else if (o->t == -1 && !is_touching(o, prot, 1, 1) &&
        (cur_beat || (!cur_beat && next_beat && beat_d >= 1 - BILLOW_ANIM)))
    {
        /* Starts appearing */
        o->t = T;
    } else {
        o->tag = OBJID_BILLOW_EMPTY;
    }
    o->w = o->h = (o->tag != OBJID_BILLOW_EMPTY ? 1 : 0);
}

static inline void billow_update_post(sobj *o, double T, sobj *prot)
{
}

static inline void spring_update_pred(sobj *o, double T, sobj *prot)
{
    if (o->t != -1 && T - o->t >= SPRING_RECOVER_DUR) {
        o->tag = OBJID_SPRING;
        o->w = 1;
        o->y = (int)o->y + 10./16;
        o->h = 6./16;
        o->t = -1;
        update_offset(o);
    }
}

static inline void spring_init(sobj *o)
{
    o->t = -SPRING_RECOVER_DUR * 2;
    spring_update_pred(o, 0, NULL);
}

static inline void spring_update_post(sobj *o, double T, sobj *prot)
{
    if (o->is_on) {
        o->tag = OBJID_SPRING_PRESS;
        o->t = T;
        o->y = (int)o->y + 13./16;
        o->h = 3./16;
        prot->vy = -SPRING_SPD;
        prot->ay = 0;
        update_offset(o);
    }
}

static inline double get_prog(sobj *o, double T)
{
    if (o->tag == OBJID_CLOUD_ONEWAY) {
        return fmod(T, o->t) / o->t;
    } else {
        double p = fabs(1 - fmod(T, 2 * o->t) / o->t);
        return 0.5 * (1 - cos(p * M_PI));
    }
}

static inline void cloud_init(sobj *o)
{
    o->tx = o->ty = -2./16;
}

static inline void cloud_update_pred(sobj *o, double T, sobj *prot)
{
    /* Update position */
    double prog = get_prog(o, T);
    o->x = o->vx + (o->ax - o->vx) * prog;
    o->y = o->vy + (o->ay - o->vy) * prog;
    /* Maybe it's being landed on? */
    if (is_landing(o, prot)) {
        /* Move the protagonist
         * Here it's assumed that every step is SIM_STEPLEN beats */
        prot->x += (o->ax - o->vx) * (prog - get_prog(o, T - SIM_STEPLEN));
    } else {
        /* Resize according to protagonist's speed */
        o->h = (prot->vy >= (o->ay - o->vy) / o->t && is_above(o, prot) ? 0.3 : 0);
    }
}

static inline void oneway_update_pred(sobj *o, double T, sobj *prot)
{
    o->h = (prot->vy >= (o->ay - o->vy) / o->t && is_above(o, prot) ? 0.3 : 0);
}

static inline void lump_update_pred(sobj *o, double T, sobj *prot)
{
    /* Update position */
    double prog = get_prog(o, T);
    o->x = o->vx + (o->ax - o->vx) * prog;
    o->y = o->vy + (o->ay - o->vy) * prog;
}

static inline void slime_update_post(sobj *o, double T, sobj *prot)
{
    if (o->is_on) {
        take_max(prot->tag, PROT_TAG_FAILURE);
        prot->t = T;
    }
}

static inline void mushroom_init(sobj *o)
{
    switch (o->tag) {
        case OBJID_MUSHROOM_B:
        case OBJID_MUSHROOM_BL:
        case OBJID_MUSHROOM_BR:
            o->y = (int)o->y + 6./16; /* Fallthrough */
        case OBJID_MUSHROOM_T:
        case OBJID_MUSHROOM_TL:
        case OBJID_MUSHROOM_TR:
            o->h = 10./16; break;
        case OBJID_MUSHROOM_R:
            o->x = (int)o->x + 6./16; /* Fallthrough */
        case OBJID_MUSHROOM_L:
            o->h = 10./16; break;
    }
}

static inline void mushroom_update_post(sobj *o, double T, sobj *prot)
{
    if (o->is_on) {
        take_max(prot->tag, PROT_TAG_FAILURE);
        prot->t = T;
    }
    if (o->tag >= OBJID_MUSHROOM_TL && o->tag <= OBJID_MUSHROOM_BR) {
        double t = o->h; o->h = o->w; o->w = t;
        /* Is right-floating? */
        if ((o->tag & 1) == (OBJID_MUSHROOM_TR & 1))
            o->x = ((int)o->x == o->x ? (int)o->x + 6./16 : (int)o->x);
        /* Is bottom-sticking? */
        if (o->tag == OBJID_MUSHROOM_BL || o->tag == OBJID_MUSHROOM_BR)
            o->y = ((int)o->y == o->y ? (int)o->y + 6./16 : (int)o->y);
    }
}

static const double REFILL_REGEN_DUR = 4;
static const double REFILL_ANIMLEN = 2;
static const int REFILL_NFRAMES = OBJID_REFILL_WAIT - OBJID_REFILL;
static const double REFILL_FRAMEFRAC = REFILL_NFRAMES / REFILL_ANIMLEN;

static inline void refill_init(sobj *o)
{
    o->x = (int)o->x + 4./16;
    o->y = (int)o->y + 4./16;
    o->w = o->h = 0;
    o->t = -1;
}

static inline void refill_update_pred(sobj *o, double T, sobj *prot)
{
    if (o->tag == OBJID_REFILL_WAIT && T - o->t >= REFILL_REGEN_DUR) o->t = -1;
    if (o->t == -1) {
        o->tag = OBJID_REFILL +
            (int)(fmod(T, REFILL_ANIMLEN) * REFILL_FRAMEFRAC);
        update_offset(o);
    }
}

static inline void refill_update_post(sobj *o, double T, sobj *prot)
{
    if (o->tag != OBJID_REFILL_WAIT && is_touching(o, prot, 8./16, 8./16)) {
        take_max(prot->tag, PROT_TAG_REFILL);
        prot->t = T;
        o->tag = OBJID_REFILL_WAIT;
        o->t = T;
    }
}

static const double PUFF_RELAX_DUR = 0.5;
static bool puff_used;

static inline void puff_init(sobj *o)
{
    o->h = 2;
}

static inline void puff_update_pred(sobj *o, double T, sobj *prot)
{
    if (!puff_used && prot->vy >= 0 &&
        (o->is_on || (o->t != -1 && is_near(o, prot))))
    {
        puff_used = true;
        prot->vy *= (1 - SIM_STEPLEN * 20);
        take_max(prot->tag, PROT_TAG_PUFF);
        prot->t = T;
        o->t = T;
        o->tag =
            o->tag < OBJID_PUFF_R ? OBJID_PUFF_L_CURL : OBJID_PUFF_R_CURL;
    } else if (o->t != -1 && T - o->t < PUFF_RELAX_DUR) {
        o->tag =
            o->tag < OBJID_PUFF_R ? OBJID_PUFF_L_AFTER : OBJID_PUFF_R_AFTER;
    } else {
        o->tag =
            o->tag < OBJID_PUFF_R ? OBJID_PUFF_L : OBJID_PUFF_R;
        o->t = -1;
    }
}

static bool mud_wet_used;

static inline void mud_wet_update_pred(sobj *o, double T, sobj *prot)
{
    if (!mud_wet_used && o->is_on) {
        mud_wet_used = true;
        prot->vx *= (1 + SIM_STEPLEN * (o->tag <= OBJID_MUD_LAST ? -80 : 20));
    }
}

static inline void disponly_init(sobj *o)
{
    o->w = o->h = 0;
}

static inline void nxstage_init(sobj *o)
{
    o->w = o->h = 0;
    o->t = -1;
}

void sobj_init(sobj *o)
{
    if (o->tag >= OBJID_BG_FIRST && o->tag <= OBJID_BG_LAST)
        bg_init(o);
    else if (o->tag >= OBJID_BILLOW && o->tag <= OBJID_BILLOW_EMPTY)
        billow_init(o);
    else if (o->tag == OBJID_SPRING || o->tag == OBJID_SPRING_PRESS)
        spring_init(o);
    else if (o->tag >= OBJID_CLOUD_FIRST && o->tag <= OBJID_CLOUD_LAST)
        cloud_init(o);
    else if (o->tag >= OBJID_MUSHROOM_FIRST && o->tag <= OBJID_MUSHROOM_LAST)
        mushroom_init(o);
    else if (o->tag >= OBJID_REFILL && o->tag <= OBJID_REFILL_WAIT)
        refill_init(o);
    else if (o->tag >= OBJID_PUFF_FIRST && o->tag <= OBJID_PUFF_LAST)
        puff_init(o);
    else if (o->tag == OBJID_DISPONLY)
        disponly_init(o);
    else if (o->tag == OBJID_NXSTAGE)
        nxstage_init(o);
}

void sobj_update_pred(sobj *o, double T, sobj *prot)
{
    if (o->tag >= OBJID_TORCH_FIRST && o->tag <= OBJID_TORCH_LAST)
        torch_update_pred(o, T, prot);
    if (o->tag >= OBJID_FRAGILE && o->tag <= OBJID_FRAGILE_EMPTY)
        fragile_update_pred(o, T, prot);
    else if (o->tag >= OBJID_BILLOW && o->tag <= OBJID_BILLOW_EMPTY)
        billow_update_pred(o, T, prot);
    else if (o->tag == OBJID_SPRING || o->tag == OBJID_SPRING_PRESS)
        spring_update_pred(o, T, prot);
    else if (o->tag >= OBJID_CLOUD_FIRST && o->tag <= OBJID_CLOUD_LAST)
        cloud_update_pred(o, T, prot);
    else if (o->tag >= OBJID_ONEWAY_FIRST && o->tag <= OBJID_ONEWAY_LAST)
        oneway_update_pred(o, T, prot);
    else if (o->tag >= OBJID_LUMP_FIRST && o->tag <= OBJID_LUMP_LAST)
        lump_update_pred(o, T, prot);
    else if (o->tag >= OBJID_REFILL && o->tag <= OBJID_REFILL_WAIT)
        refill_update_pred(o, T, prot);
    else if (o->tag >= OBJID_PUFF_FIRST && o->tag <= OBJID_PUFF_LAST)
        puff_update_pred(o, T, prot);
    else if (o->tag >= OBJID_MUD_FIRST && o->tag <= OBJID_WET_LAST)
        mud_wet_update_pred(o, T, prot);
}

void sobj_update_post(sobj *o, double T, sobj *prot)
{
    if (o->tag >= OBJID_FRAGILE && o->tag <= OBJID_FRAGILE_EMPTY)
        fragile_update_post(o, T, prot);
    else if (o->tag >= OBJID_BILLOW && o->tag <= OBJID_BILLOW_EMPTY)
        billow_update_post(o, T, prot);
    else if (o->tag == OBJID_SPRING || o->tag == OBJID_SPRING_PRESS)
        spring_update_post(o, T, prot);
    else if (o->tag >= OBJID_SLIME_FIRST && o->tag <= OBJID_SLIME_LAST)
        slime_update_post(o, T, prot);
    else if (o->tag >= OBJID_MUSHROOM_FIRST && o->tag <= OBJID_MUSHROOM_LAST)
        mushroom_update_post(o, T, prot);
    else if (o->tag >= OBJID_REFILL && o->tag <= OBJID_REFILL_WAIT)
        refill_update_post(o, T, prot);
}

bool sobj_needs_update(sobj *o)
{
    return
        (o->tag >= OBJID_TORCH_FIRST && o->tag <= OBJID_TORCH_LAST) ||
        o->tag >= 128;
}

bool sobj_needs_collision(sobj *o)
{
    return o->tag != 0 && (o->tag < OBJID_BG_FIRST || o->tag > OBJID_BG_LAST);
}

void sobj_new_round()
{
    puff_used = false;
    mud_wet_used = false;
}
