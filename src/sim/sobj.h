/* Simulation objects */

#ifndef _SIM_OBJ_H
#define _SIM_OBJ_H

#include <stdbool.h>

typedef struct _sobj {
    char tag;
    double x, y, w, h;
    double vx, vy;
    double ax, ay;

    bool is_on; /* Whether this object caused intersection at the last tick */
    double t;
} sobj;

#define OBJID_FRAGILE       30
#define OBJID_FRAGILE_FIN   33
#define OBJID_SPRING        34
#define OBJID_CLOUD_ONEWAY  35
#define OBJID_CLOUD_RTRIP   36
#define OBJID_CLOUD_FIRST   OBJID_CLOUD_ONEWAY
#define OBJID_CLOUD_LAST    OBJID_CLOUD_RTRIP

void sobj_update_pred(sobj *o, double T, sobj *prot);
void sobj_update_post(sobj *o, double T, sobj *prot);

#endif
