#include "profile_data.h"

#include <stdbool.h>
#include <stdio.h>

profile_data profile;
static bool loaded = false;

static inline int read_int(char **t)
{
    char *s = *t;
    int ret = 0, sgn = 1;
    while (*s < '0' || *s > '9') if (*(s++) == '-') sgn = -sgn;
    while (*s >= '0' && *s <= '9') ret = ret * 10 + *(s++) - '0';
    *t = s;
    return ret * sgn;
}

void profile_load()
{
    if (loaded) return;

    FILE *f = fopen("player.dat", "r");
    if (!f) return;

    int show_clock, fullscreen;
    fscanf(f, "%d,%d,%d,%d,%d\n", &profile.bgm_vol, &profile.sfx_vol,
        &show_clock, &fullscreen, &profile.av_offset);
    profile.show_clock = show_clock;
    profile.fullscreen = fullscreen;

    profile.stages = bekter_create();
    bekter(profile_stage) chap = NULL;
    while (!feof(f)) {
        char s[256];
        if (fgets(s, sizeof s, f) == NULL) break;
        if (s[0] == '#') {
            bekter_pushback(profile.stages, chap);
            chap = NULL;
            continue;
        }
        if (chap == NULL) chap = bekter_create();
        profile_stage stg;
        int i;
        char *t = s;
        bool cleared = false;
        for (i = 0; i < N_MODCOMBS; ++i) {
            if ((stg.time[i] = read_int(&t)) != -1)
                cleared = true;
        }
        stg.cleared = cleared;
        bekter_pushback(chap, stg);
    }

    fclose(f);
    loaded = true;
}

void profile_save()
{
    FILE *f = fopen("player.dat", "w");
    if (!f) return;

    fprintf(f, "%d,%d,%d,%d,%d\n", profile.bgm_vol, profile.sfx_vol,
        (int)profile.show_clock, (int)profile.fullscreen, profile.av_offset);

    int i, j, k;
    int n = bekter_size(profile.stages) / sizeof(bekter);
    for (i = 0; i < n; ++i) {
        bekter(profile_stage) chap = bekter_at(profile.stages, i, bekter);
        int m = bekter_size(chap) / sizeof(profile_stage);
        for (j = 0; j < m; ++j) {
            profile_stage *s = bekter_at_ptr(chap, j, profile_stage);
            for (k = 0; k < N_MODCOMBS; ++k)
                fprintf(f, "%d%c", s->time[k], k == N_MODCOMBS - 1 ? '\n' : ',');
        }
        fputs("#\n", f);
    }

    fclose(f);
}

profile_stage *profile_get_stage(int ch, int st)
{
    return bekter_at_ptr(bekter_at(profile.stages, ch, bekter), st, profile_stage);
}
