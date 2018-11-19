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
    union {
        double t;
        void *p;
    } data;
} sobj;

#define OBJID_FRAGILE       30
#define OBJID_FRAGILE_FIN   33
#define OBJID_SPRING        34

void sobj_update_pred(sobj *o, double T, sobj *prot);
void sobj_update_post(sobj *o, double T, sobj *prot);

#endif
