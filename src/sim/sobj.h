/* Simulation objects */

#ifndef _SIM_OBJ_H
#define _SIM_OBJ_H

typedef void (*sobj_manipulator)(double t);

typedef struct _sobj {
    char tag;
    double x, y, w, h;
    double vx, vy;
    double ax, ay;
    sobj_manipulator man;
} sobj;

#endif
