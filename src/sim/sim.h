/* Simulation */

#ifndef _SIM_H
#define _SIM_H

#include "../bekter.h"
#include "sobj.h"

extern const double SIM_GRAVITY;    /* Gravity in units/beat^2 */
extern const double SIM_STEPLEN;    /* Step length in beats */

typedef struct _sim {
    sobj prot;          /* Protagonist */
    int grows, gcols;   /* Dimensions of the grid */
    sobj *grid;         /* Map grid */
    bekter(sobj) anim;  /* Animate objects */

    double cur_time;    /* Current time in beats */
    double last_land;   /* Last landing time in beats */
} sim;

sim *sim_create(int nrows, int ncols);
void sim_drop(sim *this);

#define sim_grid(__this, __r, __c) \
    ((__this)->grid[(__this)->gcols * (__r) + (__c)])

void sim_tick(sim *this);

#endif
