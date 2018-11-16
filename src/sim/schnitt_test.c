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
    if (false < true) puts("A");
    if (true < false) puts("B");
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

    schnitt_apply(0.1, 0.1, 0.5, 0.5);
    n = 0;
    reg(0, 0);
    reg(0, 1);
    reg(1, 1);
    reg(1, 0);
    reg(0.1, 0.1);
    reg(0.1, 0.5);
    reg(0.5, 0.5);
    reg(0.5, 0.1);
    if (!schnitt_check(n, x, y)) return 1;

    n = 0;
    reg(0, 0);
    reg(0, 1);
    reg(1, 1);
    reg(1, 0);
    if (!schnitt_check(n, x, y)) return 1;

    schnitt_apply(0, 0, .5, .5);
    schnitt_apply(1, 1, .5, .5);
    n = 0;
    reg(.5, 1);
    reg(0, 1);
    reg(0, .5);
    reg(.5, .5);
    reg(.5, 0);
    reg(1, 0);
    reg(1, .5);
    if (!schnitt_check(n, x, y)) return 1;

    schnitt_apply(0, 0, .3, .3);
    schnitt_apply(.3, .3, .7, .7);
    schnitt_apply(.7, .7, 1, 1);
    n = 0;
    reg(0, 1);
    reg(0, .3);
    reg(.3, .3);
    reg(.3, 0);
    reg(.3, .7);
    reg(.7, .7);
    reg(.7, .3);
    reg(.7, 1);
    reg(1, 0);
    reg(1, .7);
    if (!schnitt_check(n, x, y)) return 1;

    schnitt_apply(0, 1, .3, .4);
    schnitt_apply(.5, 0, 1, .4);
    schnitt_apply(.5, .7, 1, 1);
    schnitt_apply(.7, .6, 1, 1);
    n = 0;
    reg(0, 0);
    reg(0, .4);
    reg(.3, .4);
    reg(.3, 1);
    reg(.5, 1);
    reg(.5, .7);
    reg(.7, .7);
    reg(.7, .6);
    reg(1, .6);
    reg(1, .4);
    reg(.5, .4);
    reg(.5, 0);
    if (!schnitt_check(n, x, y)) return 1;

    return 0;
}
