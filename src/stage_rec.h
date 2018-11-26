/* Representation of all data in a CSV stage data file */

#ifndef _STAGE_REC_H
#define _STAGE_REC_H

#include "sim/sim.h"
#include "bekter.h"
#include "resources.h"

struct _stage_dialogue {
    int r1, c1, r2, c2; /* Trigger area */
    bekter(dialogue_entry) content;
};

struct stage_rec {
    int world_r, world_c;
    sim *sim;
    int cam_r1, cam_c1, cam_r2, cam_c2;
    int spawn_r, spawn_c;

    /* String table */
    bekter(char *) strtab;

    /* Texture table */
    texture prot_tex;
    texture grid_tex[256];
#define FAILURE_NF 4
    texture prot_fail_tex[FAILURE_NF];

    /* Dialogues */
    bekter(struct _stage_dialogue) plot;
};

struct stage_rec *stage_read(const char *path);
void stage_drop(struct stage_rec *this);

#endif
