/* gcc sim/schnitt_test.c sim/schnitt.c -DSCHNITT_TEST -O2 */

#include "schnitt.h"
#include <stdio.h>
#include <time.h>

static int n;
static float x[20], y[20];

inline void reg(float _x, float _y)
{
    x[n] = _x;
    y[n++] = _y;
}

int main()
{
    /* Four corners */
    schnitt_apply(-0.5, -0.5, 0.4, 0.3);
    n = 0;
    reg(0, 0.3);
    reg(0, 1);
    reg(1, 1);
    reg(1, 0);
    reg(0.4, 0);
    reg(0.4, 0.3);
    if (!schnitt_check(n, x, y)) return 1;
    if (!schnitt_check_d(0, 0, -1)) return 1;
    if (!schnitt_check_d(1, -1, 0)) return 1;
    if (!schnitt_check_d(2, 0.4, 0)) return 1;
    if (!schnitt_check_d(3, 0, 0.3)) return 1;

    schnitt_apply(0, 0, 0.4, 0.3);
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

    schnitt_apply(0.5, 0.5, 0, 1);
    n = 0;
    reg(0, 0);
    reg(0, 0.5);
    reg(0.5, 0.5);
    reg(0.5, 1);
    reg(1, 1);
    reg(1, 0);
    if (!schnitt_check(n, x, y)) return 1;

    schnitt_apply(0.5, 0.5, 1, 0);
    if (!schnitt_check(n, y, x)) return 1;

    /* Inside */
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

    /* None */
    n = 0;
    reg(0, 0);
    reg(0, 1);
    reg(1, 1);
    reg(1, 0);
    if (!schnitt_check(n, x, y)) return 1;

    schnitt_apply(-2, -2, -1, -1);
    if (!schnitt_check(n, x, y)) return 1;

    schnitt_apply(.4, 0, .4, 1);
    if (!schnitt_check(n, x, y)) return 1;

    schnitt_apply(1, .4, 0, .4);
    if (!schnitt_check(n, x, y)) return 1;

    schnitt_apply(-5, 1, 6, 2);
    if (!schnitt_check(n, x, y)) return 1;

    schnitt_apply(5, 1, 1, 7);
    if (!schnitt_check(n, x, y)) return 1;

    /* All */
    schnitt_apply(-5, -5, 5, 5);
    n = 0;
    if (!schnitt_check(n, x, y)) return 1;

    /* Cuttings touching on the corners */
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

    /* More complex patterns */
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
    if (!schnitt_check_d(0, 0, -1)) return 1;

    schnitt_apply(0, 1, .3, .4);
    schnitt_apply(.5, 0, 1, .4);
    schnitt_apply(.3, .2, .5, .4);
    n = 0;
    reg(0, 0);
    reg(.5, 0);
    reg(.5, .2);
    reg(.3, .2);
    reg(.3, .4);
    reg(0, .4);
    reg(1, .4);
    reg(1, 1);
    reg(.3, 1);
    if (!schnitt_check(n, x, y)) return 1;

    schnitt_apply(0, 1, .3, .4);
    schnitt_apply(.5, 0, 1, .4);
    schnitt_apply(.3, .2, .5, .4);
    schnitt_apply(.1, .1, .9, .9);
    schnitt_apply(.3, 1, 1, .9);
    schnitt_apply(.9, .9, 1, .4);
    n = 0;
    reg(0, 0);
    reg(.5, 0);
    reg(.5, .1);
    reg(.1, .1);
    reg(.1, .4);
    reg(0, .4);
    if (!schnitt_check(n, x, y)) return 1;

    /* Movement tests */
    schnitt_apply(0, 0, .9, .1);
    schnitt_apply(0, 0, .2, .9);
    schnitt_check(-1, NULL, NULL);
    if (!schnitt_check_d(7, .2, .1)) return 1;

    schnitt_apply(0, 0, 1, .1);
    schnitt_apply(1, 0, .8, .9);
    schnitt_check(-1, NULL, NULL);
    if (!schnitt_check_d(6, -.2, .1)) return 1;

    schnitt_apply(0, .2, .1, 1);
    schnitt_apply(0, 1, .7, .6);
    schnitt_check(-1, NULL, NULL);
    if (!schnitt_check_d(5, .1, -.4)) return 1;

    schnitt_apply(1, 1, .9, .1);
    schnitt_apply(1, 1, .3, .8);
    schnitt_check(-1, NULL, NULL);
    if (!schnitt_check_d(4, -.1, -.2)) return 1;

    clock_t start = clock();
    int i, j;
    float dx, dy, sx = 0, sy = 0;
    for (i = 0; i < 1000000; ++i) {
        schnitt_apply(0, 1, .3, .4);
        schnitt_apply(.5, 0, 1, .4);
        schnitt_apply(.3, .2, .5, .4);
        schnitt_apply(.1, .1, .9, .9);
        schnitt_apply(.3, 1, 1, .9);
        schnitt_apply(.9, .9, 1, .4);
        for (j = 0; j < 100; ++j) {
            schnitt_apply(4, 5, 3, 6);
            schnitt_apply(-2, -5, -4, -3);
        }
        schnitt_flush(x, y);
        sx += dx; sy += dy;
    }
    printf("%.4f\n", sx + sy);
    printf("%.6lf s\n", (double)(clock() - start) / CLOCKS_PER_SEC);

    puts("*\\(^ ^)/*");
    return 0;
}
