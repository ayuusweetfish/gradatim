/* Data that are saved as configurations or profiles */

#ifndef _PROFILE_DATA_H
#define _PROFILE_DATA_H

#include "bekter.h"

typedef struct _profile_stage {
    int time;
    int retries;
} profile_stage;

typedef struct _profile_data {
    int bgm_vol;
    int sfx_vol;
    bekter(bekter(profile_stage)) stages;
} profile_data;

extern profile_data profile;

void profile_load();
void profile_save();

#endif
