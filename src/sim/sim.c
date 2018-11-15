#include "sim.h"
#include <stdlib.h>

const double SIM_GRAVITY = 3.5 * 1.414213562;
const double SIM_STEPLEN = 0.001;
static const double MAX_VY = 8;

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

void sim_tick(sim *this)
{
    this->prot.vx += this->prot.ax * SIM_STEPLEN;
    this->prot.vy += (this->prot.ay + SIM_GRAVITY) * SIM_STEPLEN;
    if (this->prot.vy >= MAX_VY) this->prot.vy = MAX_VY;
    this->prot.x += this->prot.vx * SIM_STEPLEN;
    this->prot.y += this->prot.vy * SIM_STEPLEN;
}
