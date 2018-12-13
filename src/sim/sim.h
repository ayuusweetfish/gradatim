/* Simulation */

#ifndef _SIM_H
#define _SIM_H

#include "../bekter.h"
#include "sobj.h"

#include <stdbool.h>

#define SIM_GRAVITY (4 * 1.414213562)   /* Gravity in units/beat^2 */
#define SIM_STEPLEN 0.001               /* Step length in beats */

typedef struct _sim {
    sobj prot;          /* Protagonist */
    int worldr, worldc; /* Offset to the whole world */
    int grows, gcols;   /* Dimensions of the grid */
    sobj *grid;         /* Map grid */
    bool grid_initialized;

    sobj **anim;        /* List of moving & extra objects */
    int anim_sz, anim_cap;
    sobj **block;       /* List of extra objects that need collision detection */
    int block_sz, block_cap;
    sobj **volat;       /* List of stuff that need to be updated frequently */
    int volat_sz, volat_cap;

    double cur_time;    /* Current time in beats */
    double last_land;   /* Last landing time in beats */
} sim;

sim *sim_create(int nrows, int ncols);
void sim_drop(sim *this);

#define sim_grid(__this, __r, __c) \
    ((__this)->grid[(__this)->gcols * (__r) + (__c)])

void sim_add(sim *this, sobj *o);
void sim_check_volat(sim *this, sobj *o);
void sim_reinit(sim *this);
void sim_tick(sim *this);
bool sim_prophecy(sim *this, double time);

#endif
