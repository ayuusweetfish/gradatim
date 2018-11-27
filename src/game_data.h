/* Representation of all data in a CSV stage/chapter data file */

#ifndef _GAME_DATA_H
#define _GAME_DATA_H

#include "sim/sim.h"
#include "bekter.h"
#include "resources.h"

/* For stages */

typedef struct _stage_dialogue {
    int r1, c1, r2, c2; /* Trigger area */
    bekter(dialogue_entry) content;
} stage_dialogue;

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
    int plot_ct;
};

struct stage_rec *stage_read(const char *path);
void stage_drop(struct stage_rec *this);

/* For chapters */

struct _chap_track {
    char src_id;    /* Index of source track; -1 if reading from file */
    char *str;      /* File name or filter name */
    double arg;     /* Offset or filter argument */
};

struct chap_rec {
    /* Beats per minute for all BG music */
    int bpm;
    /* Beat length in seconds; calculated from `bpm` */
    double beat;
    /* Beats per measure */
    int sig;
    /* Bitmask denoting availability of dash at each beat */
    unsigned int dash_mask;
    /* Bitmask denoting availability of hop at each beat */
    unsigned int hop_mask;
    /* Tracks */
#define MAX_CHAP_TRACKS 8
    struct _chap_track tracks[MAX_CHAP_TRACKS];
    int n_tracks;
    /* Stages */
#define MAX_CHAP_STAGES 64
    struct stage_rec *stages[MAX_CHAP_STAGES];
    int n_stages;
};

struct chap_rec *chap_read(const char *path);
void chap_drop(struct chap_rec *this);

#endif
