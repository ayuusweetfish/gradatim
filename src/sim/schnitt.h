/* Axis-aligned polygon intersection */

#ifndef _SCHNITT_H
#define _SCHNITT_H

void schnitt_apply(float x1, float y1, float x2, float y2);
void schnitt_flush(float *dx, float *dy);

#ifdef SCHNITT_TEST
#include <stdbool.h>
bool schnitt_check(int n, float *x, float *y);
bool schnitt_check_d(int dir, float dx, float dy);
#endif

#endif
