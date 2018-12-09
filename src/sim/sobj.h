/* Simulation objects */

#ifndef _SIM_OBJ_H
#define _SIM_OBJ_H

#include <stdbool.h>

typedef struct _sobj {
    unsigned char tag;
    double x, y, w, h;
    double vx, vy;
    double ax, ay;
    double tx, ty;  /* Offset of texture */

    bool is_on; /* Whether this object caused intersection at the last tick */
    double t;
} sobj;

#define OBJID_BG_FIRST      1
#define OBJID_TORCH_FIRST   10
#define OBJID_TORCH_LAST    15
#define OBJID_BG_LAST       15

#define OBJID_DRAW_AFTER    32

#define OBJID_FRAGILE       128
#define OBJID_FRAGILE_EMPTY 132
#define OBJID_BILLOW        133
#define OBJID_BILLOW_EMPTY  138
#define OBJID_SPRING        139
#define OBJID_SPRING_PRESS  140
#define OBJID_CLOUD_FIRST   141
#define OBJID_CLOUD_ONEWAY  141
#define OBJID_CLOUD_RTRIP   142
#define OBJID_CLOUD_LAST    142
#define OBJID_ONEWAY_FIRST  143
#define OBJID_ONEWAY_LAST   145
#define OBJID_LUMP_FIRST    146
#define OBJID_SLIME_FIRST   150
#define OBJID_SLIME_LAST    151
#define OBJID_LUMP_LAST     151
#define OBJID_MUSHROOM_FIRST    152
#define OBJID_MUSHROOM_T    152
#define OBJID_MUSHROOM_B    153
#define OBJID_MUSHROOM_L    154
#define OBJID_MUSHROOM_R    155
#define OBJID_MUSHROOM_TL   156
#define OBJID_MUSHROOM_TR   157
#define OBJID_MUSHROOM_BL   158
#define OBJID_MUSHROOM_BR   159
#define OBJID_MUSHROOM_LAST 159
#define OBJID_REFILL        160
#define OBJID_REFILL_WAIT   169
#define OBJID_PUFF_FIRST    170
#define OBJID_PUFF_L        170
#define OBJID_PUFF_L_CURL   171
#define OBJID_PUFF_L_AFTER  172
#define OBJID_PUFF_R        173
#define OBJID_PUFF_R_CURL   174
#define OBJID_PUFF_R_AFTER  175
#define OBJID_PUFF_LAST     175
#define OBJID_MUD_FIRST     176
#define OBJID_MUD_LAST      178
#define OBJID_WET_FIRST     179
#define OBJID_WET_LAST      181

#define OBJID_DISPONLY      254
#define OBJID_NXSTAGE       255

#define PROT_TAG_FAILURE    9
#define PROT_TAG_NXSTAGE    8
#define PROT_TAG_REFILL     7
#define PROT_TAG_PUFF       6

void sobj_init(sobj *o);
bool sobj_needs_update(sobj *o);
void sobj_update_pred(sobj *o, double T, sobj *prot);
void sobj_update_post(sobj *o, double T, sobj *prot);
void sobj_new_round();

#endif
