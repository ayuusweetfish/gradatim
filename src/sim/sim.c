#include "sim.h"
#include "schnitt.h"
#include <stdlib.h>

const double SIM_GRAVITY = 3.5 * 1.414213562;
const double SIM_STEPLEN = 0.001;
static const double MAX_VY = 8;

sim *sim_create(int grows, int gcols)
{
    sim *ret = malloc(sizeof(sim));
    ret->grows = grows;
    ret->gcols = gcols;
    ret->grid = malloc(grows * gcols * sizeof(sobj));
    int i, j;
    for (i = 0; i < grows; ++i)
        for (j = 0; j < gcols; ++j) {
            sim_grid(ret, i, j).x = j;
            sim_grid(ret, i, j).y = i;
        }
    return ret;
}

void sim_drop(sim *this)
{
    free(this->grid);
    bekter_drop(this->anim);
    free(this);
}

/* Checks for intersections; `schnitt_flush()` needs be called afterwards
 * if `inst` is true, the function will return on first intersection */
static inline bool check_intsc(sim *this, bool inst)
{
    bool in = false;
    /* Neighbouring cells */
    int px = (int)this->prot.x, py = (int)this->prot.y;
    sobj *o = &sim_grid(this, py, px);
    if (o->tag != 0) {
        in |= schnitt_apply(o->x - this->prot.x, o->y - this->prot.y, 1, 1);
        if (inst && in) return true;
    }
    if (px + 1 < this->gcols) {
        o = &sim_grid(this, py, px + 1);
        if (o->tag != 0) {
            in |= schnitt_apply(o->x - this->prot.x, o->y - this->prot.y, 1, 1);
            if (inst && in) return true;
        }
    }
    if (py + 1 < this->grows) {
        o = &sim_grid(this, py + 1, px);
        if (o->tag != 0) {
            in |= schnitt_apply(o->x - this->prot.x, o->y - this->prot.y, 1, 1);
            if (inst && in) return true;
        }
    }
    if (px + 1 < this->gcols && py + 1 < this->grows) {
        o = &sim_grid(this, py + 1, px + 1);
        if (o->tag != 0) {
            in |= schnitt_apply(o->x - this->prot.x, o->y - this->prot.y, 1, 1);
            if (inst && in) return true;
        }
    }
    return in;
}

void sim_tick(sim *this)
{
    this->prot.vx += this->prot.ax * SIM_STEPLEN;
    this->prot.vy += (this->prot.ay + SIM_GRAVITY) * SIM_STEPLEN;
    if (this->prot.vy >= MAX_VY) this->prot.vy = MAX_VY;
    this->prot.x += this->prot.vx * SIM_STEPLEN;
    this->prot.y += this->prot.vy * SIM_STEPLEN;

    /* Respond by movement */
    float x0 = this->prot.x, y0 = this->prot.y;
    bool in = check_intsc(this, false);
    float dx[8], dy[8];
    schnitt_flush(dx, dy);
    if (in) {
        int i;
        for (i = 0; i < 4; ++i) {
            this->prot.x = x0 + dx[i];
            this->prot.y = y0 + dy[i];
            in = check_intsc(this, true);
            if (!in) {
                if (i == 0 || i == 3) this->prot.vy = 0;
                else this->prot.vx = 0;
                break;
            }
        }
        if (in) {
            float min = 10;
            int dir = -1;
            for (i = 5; i < 8; ++i) {
                this->prot.x = x0 + dx[i];
                this->prot.y = y0 + dy[i];
                in = check_intsc(this, true);
                if (!in && dx[i] * dx[i] + dy[i] * dy[i] < min) {
                    min = dx[i] * dx[i] + dy[i] * dy[i];
                    dir = i;
                }
            }
            if (dir == -1) {
                puts("> <");
            } else {
                this->prot.x = x0 + dx[dir];
                this->prot.y = y0 + dy[dir];
                this->prot.vx = this->prot.vy = 0;
            }
        }
    }
}
