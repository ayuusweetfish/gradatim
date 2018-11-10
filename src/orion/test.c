/* gcc orion.c test.c libs_wrapper.o -O2 -F /Library/Frameworks -framework SDL2 -lvorbisfile -lsoundtouch -liir -lc++ */
#include "orion.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    struct orion o = orion_create(44100, 2);
    const char *msg = orion_load_ogg(&o, 0, "sketchch.ogg");
    if (msg != NULL) {
        puts(msg);
        return 1;
    }

    orion_apply_lowpass(&o, 0, 1, 880);

    int i;
    for (i = 0; i <= 1; ++i) {
        orion_overall_play(&o);
        if (i == 0) {
            orion_play_once(&o, i);
            sleep(2);
            orion_seek(&o, i, -44100 * 3);
            sleep(2);
            printf("%d\n", orion_tell(&o, i));
            sleep(2);
            printf("%d\n", orion_tell(&o, i));
        } else {
            orion_play_loop(&o, i, 0, 44100, 88200);
            sleep(5);
        }
        orion_pause(&o, i);
        orion_overall_pause(&o);
        sleep(1);
    }

    return 0;
}
