/* Axis-aligned polygon intersection */

#ifndef _SCHNITT_H
#define _SCHNITT_H

#include <stdbool.h>

bool schnitt_apply(double x1, double y1, double x2, double y2);
void schnitt_flush(double *dx, double *dy);

#ifdef SCHNITT_TEST
bool schnitt_check(int n, double *x, double *y);
bool schnitt_check_d(int dir, double dx, double dy);
#endif

#endif
