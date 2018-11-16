/* gcc sim/schnitt_test.c sim/schnitt.c -DSCHNITT_TEST -O2 */

#include "schnitt.h"
#include <stdio.h>

static int n;
static float x[20], y[20];

inline void reg(float _x, float _y)
{
    x[n] = _x;
    y[n++] = _y;
}

int main()
{
    schnitt_apply(-0.5, -0.5, 0.5, 0.5);
    n = 0;
    reg(0, 0.5);
    reg(0, 1);
    reg(1, 1);
    reg(1, 0);
    reg(0.5, 0);
    reg(0.5, 0.5);
    if (!schnitt_check(n, x, y)) return 1;

    schnitt_apply(0, 0, 0.5, 0.5);
    if (!schnitt_check(n, x, y)) return 1;

    schnitt_apply(0.5, 0.5, 1, 1);
    n = 0;
    reg(0, 0);
    reg(0, 1);
    reg(0.5, 1);
    reg(0.5, 0.5);
    reg(1, 0.5);
    reg(1, 0);
    if (!schnitt_check(n, x, y)) return 1;

    schnitt_apply(1e-8, 1e-8, 0.5, 0.5);
    n = 0;
    reg(0, 0);
    reg(0, 1);
    reg(1, 1);
    reg(1, 0);
    reg(1e-8, 1e-8);
    reg(1e-8, 0.5);
    reg(0.5, 0.5);
    reg(0.5, 1e-8);
    if (!schnitt_check(n, x, y)) return 1;

    return 0;
}
