#include "profile_data.h"

#include <stdbool.h>
#include <stdio.h>

profile_data profile;
static bool loaded = false;

void profile_load()
{
    if (loaded) return;

    FILE *f = fopen("player.dat", "r");
    if (!f) return;

    fscanf(f, "%d,%d\n", &profile.bgm_vol, &profile.sfx_vol);

    profile.stages = bekter_create();
    bekter(profile_stage) chap = NULL;
    while (!feof(f)) {
        char s[256];
        fgets(s, sizeof s, f);
        if (s[0] == '#') {
            bekter_pushback(profile.stages, chap);
            chap = NULL;
            continue;
        }
        if (chap == NULL) chap = bekter_create();
        profile_stage stg;
        sscanf(s, "%d,%d", &stg.time, &stg.retries);
        bekter_pushback(chap, stg);
    }

    fclose(f);
    loaded = true;
}

void profile_save()
{
}

profile_stage *profile_get_stage(int ch, int st)
{
    return bekter_at_ptr(bekter_at(profile.stages, ch, bekter), st, profile_stage);
}
