#include "../orion.h"

#include <stdio.h>
#include <stdlib.h>

int main()
{
    struct orion o = orion_create(44100, 2);
    const char *msg = orion_load_ogg(&o, 0, "sketchch.ogg");
    if (msg != NULL) {
        puts(msg);
        return 1;
    }

    orion_overall_play(&o);
    orion_play_loop(&o, 0, 0, 0, -1);

    double beat = 1.0 / 3;
    double resolution = 50;
    printf("Resolution: %.1lf ms\n", beat / (resolution * 2) * 1000);

    while (getchar() == '\n') {
        double time = orion_tell(&o, 0) / 44100.0;
        double t1 = fmod(time, beat), t2 = beat - t1;
        if (t1 > t2) t1 = -t2;
        int x = (int)(t1 / beat * (resolution * 2) + 0.5);
        int i;
        for (i = -resolution; i <= resolution; ++i)
            putchar(i == x ? '*' : (i == 0 ? '|' : (abs(i) <= resolution / 3 ? '-' : ' ')));
    }

    orion_overall_pause(&o);
    return 0;
}
