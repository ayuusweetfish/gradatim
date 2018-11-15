/* Simulation */

#ifndef _SIM_H
#define _SIM_H

#include "../bekter.h"
#include "sobj.h"

extern const double SIM_GRAVITY;

typedef struct _sim {
    double step;        /* Step length in units */
    sobj prot;          /* Protagonist */
    int grows, gcols;   /* Dimensions of the grid */
    sobj *grid;         /* Map grid */
    bekter(sobj) anim;  /* Animate objects */
} sim;

sim *sim_create(int nrows, int ncols);
void sim_drop(sim *this);

void sim_tick(sim *this);

#endif
