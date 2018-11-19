#include "sobj.h"
#include "sim.h"
#include <math.h>
#include <stdbool.h>

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

static inline void cloud_update_post(sobj *o, double T, sobj *prot)
{
}

void sobj_update_pred(sobj *o, double T, sobj *prot)
{
    if (o->tag >= OBJID_FRAGILE && o->tag <= OBJID_FRAGILE_EMPTY)
        fragile_update_pred(o, T, prot);
    else if (o->tag == OBJID_SPRING || o->tag == OBJID_SPRING_PRESS)
        spring_update_pred(o, T, prot);
    else if (o->tag >= OBJID_CLOUD_FIRST && o->tag <= OBJID_CLOUD_LAST)
        cloud_update_pred(o, T, prot);
}

void sobj_update_post(sobj *o, double T, sobj *prot)
{
    if (o->tag >= OBJID_FRAGILE && o->tag <= OBJID_FRAGILE_EMPTY)
        fragile_update_post(o, T, prot);
    else if (o->tag == OBJID_SPRING || o->tag == OBJID_SPRING_PRESS)
        spring_update_post(o, T, prot);
    else if (o->tag >= OBJID_CLOUD_FIRST && o->tag <= OBJID_CLOUD_LAST)
        cloud_update_post(o, T, prot);
}

bool sobj_needs_update(sobj *o)
{
    return o->tag >= 30;
}
