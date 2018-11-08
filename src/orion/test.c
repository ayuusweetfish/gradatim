/* gcc orion.c test.c libs_wrapper.o -O2 -F /Library/Frameworks -framework SDL2 -lvorbisfile -lsoundtouch -liir -lc++ */
#include "orion.h"

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

    return 0;
}
