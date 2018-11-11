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

    orion_apply_lowpass(&o, 0, 1, 880);
    orion_apply_stretch(&o, 0, 2, -20);

    while (1) {
        int a = rand() % 9;
        printf("%d\n", a);
        switch (a) {
            case 0: orion_overall_play(&o); break;
            case 1: orion_overall_pause(&o); break;
            case 2: orion_play_once(&o, rand() % 3); break;
            case 3: orion_play_loop(&o, rand() % 3, rand(), rand(), rand()); break;
            case 4: orion_play_loop(&o, rand() % 3, rand(), rand(), -1); break;
            case 5: orion_pause(&o, rand() % 3); break;
            case 6: orion_resume(&o, rand() % 3); break;
            case 7: orion_seek(&o, rand() % 3, rand()); break;
            case 8:
                orion_ramp(&o, rand() % 3,
                    (double)rand() / RAND_MAX, (double)rand() / RAND_MAX);
                break;
        }
    }

    return 0;
}
