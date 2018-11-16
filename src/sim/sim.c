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
            sim_grid(ret, i, j).w = sim_grid(ret, i, j).h = 1;
        }
    return ret;
}

void sim_drop(sim *this)
{
    free(this->grid);
    bekter_drop(this->anim);
    free(this);
}

/* Sends a rectangle to schnitt for checking. */
static inline bool apply_intsc(sim *this, sobj *o)
{
    return schnitt_apply(
        o->x - this->prot.x,
        o->y - this->prot.y,
        o->x - this->prot.x + o->w,
        o->y - this->prot.y + o->h);
}

/* Checks for intersections; **`schnitt_flush()` must be called afterwards**
 * if `inst` is true, the function will return on first intersection */
static inline bool check_intsc(sim *this, bool inst)
{
    bool in = false;
    /* Neighbouring cells */
    int px = (int)this->prot.x, py = (int)this->prot.y;
    sobj *o = &sim_grid(this, py, px);
    if (o->tag != 0) {
        in |= apply_intsc(this, o);
        if (inst && in) return true;
    }
    if (px + 1 < this->gcols) {
        o = &sim_grid(this, py, px + 1);
        if (o->tag != 0) {
            in |= apply_intsc(this, o);
            if (inst && in) return true;
        }
    }
    if (py + 1 < this->grows) {
        o = &sim_grid(this, py + 1, px);
        if (o->tag != 0) {
            in |= apply_intsc(this, o);
            if (inst && in) return true;
        }
    }
    if (px + 1 < this->gcols && py + 1 < this->grows) {
        o = &sim_grid(this, py + 1, px + 1);
        if (o->tag != 0) {
            in |= apply_intsc(this, o);
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
    float dx[16], dy[16];
    schnitt_flush(dx, dy);
    if (in) {
        int i, dir = -1;
        float min = 10;
        for (i = 0; i < 4; ++i) {
            this->prot.x = x0 + dx[i];
            this->prot.y = y0 + dy[i];
            in = check_intsc(this, true);
            schnitt_flush(NULL, NULL);
            if (!in && dx[i] * dx[i] + dy[i] * dy[i] < min) {
                min = dx[i] * dx[i] + dy[i] * dy[i];
                dir = i;
            }
        }
        if (!in) {
            if (dir == 0 || dir == 3) this->prot.vy = 0;
            else this->prot.vx = 0;
        } else {
            for (i = 4; i < 8; ++i) {
                this->prot.x = x0 + dx[i];
                this->prot.y = y0 + dy[i];
                in = check_intsc(this, true);
                schnitt_flush(NULL, NULL);
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
                if (dir == 1 || dir == 2 || dir >= 4) this->prot.vx = 0;
                if (dir == 0 || dir == 3 || dir >= 4) this->prot.vy = 0;
            }
        }
    }
}
