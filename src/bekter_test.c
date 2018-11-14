/* gcc bekter_test.c bekter.c -O2 */
#include "bekter.h"
#include <stdio.h>

struct big_data {
    int val, unused[10];
};

int main()
{
    bekter(int / struct big_data) b = bekter_create();

    int i, x;
    struct big_data y;

    for (i = 0; i < 1000; ++i)
        bekter_pushback(b, ((struct big_data){i, 0, i + 10}));
    for bekter_each(b, i, y) printf("%d %d\n", y.val, y.unused[1]);
    bekter_clear(b);

    for (i = 0; i < 1000; ++i) {
        bekter_pushback(b, i);
        if (i * (i + 1) % 10 == 0) {
            bekter_popback(b, x); printf("%d\n", x);
            bekter_popback(b, x); printf("%d\n", x);
        }
    }
    for bekter_each(b, i, x) printf("%d\n", x);
    printf("%d\n", bekter_at(b, 19, int));

    return 0;
}
