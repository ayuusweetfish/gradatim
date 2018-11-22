/* Simulation objects */

#ifndef _SIM_OBJ_H
#define _SIM_OBJ_H

#include <stdbool.h>

typedef struct _sobj {
    char tag;
    double x, y, w, h;
    double vx, vy;
    double ax, ay;
    double tx, ty;  /* Offset of texture */

    bool is_on; /* Whether this object caused intersection at the last tick */
    double t;
} sobj;

#define OBJID_DRAW_AFTER    10

#define OBJID_FRAGILE       30
#define OBJID_FRAGILE_EMPTY 34
#define OBJID_SPRING        35
#define OBJID_SPRING_PRESS  36
#define OBJID_CLOUD_ONEWAY  37
#define OBJID_CLOUD_RTRIP   38
#define OBJID_CLOUD_FIRST   OBJID_CLOUD_ONEWAY
#define OBJID_CLOUD_LAST    OBJID_CLOUD_RTRIP

#define OBJID_MUSHROOM_T    39
#define OBJID_MUSHROOM_B    40
#define OBJID_MUSHROOM_L    41
#define OBJID_MUSHROOM_R    42
#define OBJID_MUSHROOM_TL   43
#define OBJID_MUSHROOM_TR   44
#define OBJID_MUSHROOM_BL   45
#define OBJID_MUSHROOM_BR   46
#define OBJID_MUSHROOM_FIRST OBJID_MUSHROOM_T
#define OBJID_MUSHROOM_LAST  OBJID_MUSHROOM_BR

#define OBJID_REFILL        47
#define OBJID_REFILL_WAIT   51

#define OBJID_PUFF_L        52
#define OBJID_PUFF_L_CURL   53
#define OBJID_PUFF_L_AFTER  54
#define OBJID_PUFF_R        55
#define OBJID_PUFF_R_CURL   56
#define OBJID_PUFF_R_AFTER  57
#define OBJID_PUFF_FIRST    OBJID_PUFF_L
#define OBJID_PUFF_LAST     OBJID_PUFF_R_AFTER

#define OBJID_NXSTAGE       127

#define PROT_TAG_FAILURE    9
#define PROT_TAG_NXSTAGE    8
#define PROT_TAG_REFILL     7
#define PROT_TAG_PUFF       6

void sobj_init(sobj *o);
bool sobj_needs_update(sobj *o);
void sobj_update_pred(sobj *o, double T, sobj *prot);
void sobj_update_post(sobj *o, double T, sobj *prot);
void sobj_trigger(sobj *o, double T, sobj *prot);

#endif
