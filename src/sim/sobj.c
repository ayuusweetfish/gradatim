#include "sobj.h"
#include "sim.h"
#include <math.h>
#include <stdbool.h>

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

static inline bool is_touching(sobj *o, sobj *prot, double w, double h)
{
    return (in_range(prot->x, o->x, o->x + w) ||
        in_range(prot->x + prot->w, o->x, o->x + w)) &&
        (in_range(prot->y, o->y, o->y + h) ||
        in_range(prot->y + prot->h, o->y, o->y + h));
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

static inline void spring_update_pred(sobj *o, double T, sobj *prot)
{
    if (o->t != -1 && T - o->t >= SPRING_RECOVER_DUR) {
        o->tag = OBJID_SPRING;
        o->w = 1;
        o->y = (int)o->y + 10./16;
        o->h = 6./16;
        o->t = -1;
    }
}

static inline void spring_init(sobj *o)
{
    o->t = -SPRING_RECOVER_DUR * 2;
    spring_update_pred(o, 0, NULL);
}

static inline void spring_update_post(sobj *o, double T, sobj *prot)
{
    if (is_landing(o, prot)) {
        o->tag = OBJID_SPRING_PRESS;
        o->t = T;
        o->y = (int)o->y + 13./16;
        o->h = 3./16;
        prot->vy = -SPRING_SPD;
        prot->ay = 0;
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

static const double REFILL_FRM1 = 4./3;
static const double REFILL_FRM2 = 6./3;
static const double REFILL_FRM3 = 10./3;
static const double REFILL_FRM4 = 12./3;
static const double REFILL_REGEN_DUR = 4;

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
        double phase = fmod(T, REFILL_FRM4);
        if (phase < REFILL_FRM1) o->tag = OBJID_REFILL;
        else if (phase < REFILL_FRM2) o->tag = OBJID_REFILL + 1;
        else if (phase < REFILL_FRM3) o->tag = OBJID_REFILL + 2;
        else o->tag = OBJID_REFILL + 3;
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

static inline void nxstage_init(sobj *o)
{
    o->w = o->h = 0;
}

void sobj_init(sobj *o)
{
    if (o->tag == OBJID_SPRING || o->tag == OBJID_SPRING_PRESS)
        spring_init(o);
    else if (o->tag >= OBJID_CLOUD_FIRST && o->tag <= OBJID_CLOUD_LAST)
        cloud_init(o);
    else if (o->tag >= OBJID_MUSHROOM_FIRST && o->tag <= OBJID_MUSHROOM_LAST)
        mushroom_init(o);
    else if (o->tag >= OBJID_REFILL && o->tag <= OBJID_REFILL_WAIT)
        refill_init(o);
    else if (o->tag == OBJID_NXSTAGE)
        nxstage_init(o);
}

void sobj_update_pred(sobj *o, double T, sobj *prot)
{
    if (o->tag >= OBJID_FRAGILE && o->tag <= OBJID_FRAGILE_EMPTY)
        fragile_update_pred(o, T, prot);
    else if (o->tag == OBJID_SPRING || o->tag == OBJID_SPRING_PRESS)
        spring_update_pred(o, T, prot);
    else if (o->tag >= OBJID_CLOUD_FIRST && o->tag <= OBJID_CLOUD_LAST)
        cloud_update_pred(o, T, prot);
    else if (o->tag >= OBJID_REFILL && o->tag <= OBJID_REFILL_WAIT)
        refill_update_pred(o, T, prot);
}

void sobj_update_post(sobj *o, double T, sobj *prot)
{
    if (o->tag >= OBJID_FRAGILE && o->tag <= OBJID_FRAGILE_EMPTY)
        fragile_update_post(o, T, prot);
    else if (o->tag == OBJID_SPRING || o->tag == OBJID_SPRING_PRESS)
        spring_update_post(o, T, prot);
    else if (o->tag >= OBJID_MUSHROOM_FIRST && o->tag <= OBJID_MUSHROOM_LAST)
        mushroom_update_post(o, T, prot);
    else if (o->tag >= OBJID_REFILL && o->tag <= OBJID_REFILL_WAIT)
        refill_update_post(o, T, prot);
}

bool sobj_needs_update(sobj *o)
{
    return o->tag >= 30;
}
