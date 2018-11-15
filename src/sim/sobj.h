/* Simulation objects */

#ifndef _SIM_OBJ_H
#define _SIM_OBJ_H

typedef void (*sobj_manipulator)(float t);

typedef struct _sobj {
    char tag;
    float x, y, w, h;
    float vx, vy;
    float ax, ay;
    sobj_manipulator man;
} sobj;

#endif
