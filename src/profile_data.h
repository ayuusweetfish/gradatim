/* Data that are saved as configurations or profiles */

#ifndef _PROFILE_DATA_H
#define _PROFILE_DATA_H

#include "bekter.h"
#include "mod.h"

#include <stdbool.h>

typedef struct _profile_stage {
    int time[N_MODCOMBS];
    bool cleared;
} profile_stage;

#define VOL_RESOLUTION 20
#define VOL_VALUE (1.0 / VOL_RESOLUTION)
typedef struct _profile_data {
    int bgm_vol;
    int sfx_vol;
    bool show_clock;
    bool fullscreen;
    int av_offset;
    bekter(bekter(profile_stage)) stages;
} profile_data;

extern profile_data profile;

void profile_load();
void profile_save();
profile_stage *profile_get_stage(int ch, int st);

#endif
