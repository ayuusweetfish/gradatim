#include "sim.h"
#include <stdlib.h>

const double SIM_GRAVITY = 1.414213562;

sim *sim_create(int nrows, int ncols)
{
    sim *ret = malloc(sizeof(sim));
    ret->grows = nrows;
    ret->gcols = ncols;
    ret->grid = malloc(nrows * ncols * sizeof(sobj));
    return ret;
}

void sim_drop(sim *this)
{
    free(this->grid);
    bekter_drop(this->anim);
    free(this);
}

#define sim_grid(__this, __r, __c) \
    ((__this)->grid[(__this)->gcols * (__r) + (__c)])

void sim_tick(sim *this)
{
}
