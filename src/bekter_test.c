/* gcc bekter_test.c bekter.c -O2 */
#include "bekter.h"
#include <stdio.h>

int main()
{
    bekter b = bekter_create();

    int i, x;
    for (i = 0; i < 1000; ++i) {
        bekter_pushback(b, i);
        if (i * (i + 1) % 10 == 0) {
            bekter_popback(b, x); printf("%d\n", x);
            bekter_popback(b, x); printf("%d\n", x);
        }
    }
    for bekter_each(b, i, x) printf("%d\n", x);

    return 0;
}
