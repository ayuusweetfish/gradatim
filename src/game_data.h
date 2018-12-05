/* Representation of all data in a CSV stage/chapter data file */

#ifndef _GAME_DATA_H
#define _GAME_DATA_H

#include "sim/sim.h"
#include "sim/sobj.h"
#include "bekter.h"
#include "resources.h"

/* For stages */

typedef struct _stage_dialogue {
    int r1, c1, r2, c2; /* Trigger area */
    bekter(dialogue_entry) content;
} stage_dialogue;

typedef struct _stage_hint {
    int r, c;
    char *str, *key;
    char *img;
    /* `mul` is the multiplier for beats */
    /* `mask` denotes on which beats the image is blinked */
    int mul, mask;
} stage_hint;

struct _stage_audsrc {
    char tid;
    int r, c;
};

struct stage_rec {
    int world_r, world_c;
    int n_rows, n_cols;
    int cam_r1, cam_c1, cam_r2, cam_c2;
    int spawn_r, spawn_c;

    /* Grid data */
    char *grid;
    /* Animate & extra objects */
    int n_anim;
    sobj *anim;

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

    /* Audio sources */
#define MAX_AUD_SOURCES 8
    struct _stage_audsrc aud[MAX_AUD_SOURCES];
    int aud_ct;

    /* Text hints */
#define MAX_HINTS   8
    stage_hint hints[MAX_HINTS];
    int hint_ct;
};

struct stage_rec *stage_read(const char *path);
sim *stage_create_sim(struct stage_rec *this);
void stage_drop(struct stage_rec *this);

/* For chapters */

struct _chap_track {
    char src_id;    /* Index of source track; -1 if reading from file */
    char *str;      /* File name or filter name */
    double arg;     /* Offset or filter argument */
};

struct chap_rec {
    /* Colours in the overworld */
    int r1, g1, b1, r2, g2, b2;
    /* Index */
    int idx;
    /* Title */
    char *title;
    /* Beats per minute for all BG music */
    int bpm;
    /* Beat length in seconds; calculated from `bpm` */
    double beat;
    /* Multiplier for simulation and display;
     * This is only useful under irregular time signatures (e.g. 14/8) */
    int beat_mul;
    /* Beats per measure */
    int sig;
    /* Audio offset for all tracks */
    double offs;
    /* Number of beats per loop */
    int loop;
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
