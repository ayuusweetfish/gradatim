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
    double t;   /* Any special record; usually time */
} sobj;

#define OBJID_FRAGILE       30
#define OBJID_FRAGILE_FIN   33

void sobj_update(sobj *o, double T);

#endif
