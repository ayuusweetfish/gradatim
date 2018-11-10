#ifndef _ORION_H
#define _ORION_H

#include "libs_wrapper.h"

#include <SDL.h>

/* Type of samples */
typedef signed short orion_smp;
/* Number of tracks available */
#define ORION_NUM_TRACKS    16

enum orion_playstate {
    ORION_UNINIT = 0,
    ORION_STOPPED,
    ORION_ONCE,
    ORION_LOOP
};

struct orion_track {
    /* About the audio data */
    int nch;        /* Number of channels */
    int len;        /* Number of samples; a sample has `nch` values */
    orion_smp *pcm; /* Raw sample data; channels interleaved */

    /* About the usual playback */
    int play_pos;   /* Current playback position; in samples */
    float volume;   /* Current playback volume */

    /* About loops and ramps */
    enum orion_playstate state; /* Current playback state */
    int loop_start, loop_end;   /* A-B repeat markers; in samples */
    int ramp_end;               /* Time until end of the ramp; in samples */
    double ramp_slope;          /* The ramp slope; in 1/sample */
};

struct orion {
    int srate;      /* Sample rate of all tracks */
    int nch;        /* Number of channels; all tracks should have `nch` or 1 */
    unsigned char is_playing;
    struct orion_track track[ORION_NUM_TRACKS];
    SDL_SpinLock lock;
    SDL_Thread *playback_thread;
};

struct orion orion_create(int srate, int nch);
void orion_drop(struct orion *o);

const char *orion_load_ogg(struct orion *o, int tid, const char *path);
void orion_apply_lowpass(struct orion *o, int tid, int did, double cutoff);
void orion_apply_stretch(struct orion *o, int tid, int did, double delta_pc);
void orion_play_once(struct orion *o, int tid);
void orion_play_loop(struct orion *o, int tid, int intro_pos, int start_pos, int end_pos);
void orion_pause(struct orion *o, int tid);
void orion_resume(struct orion *o, int tid);
void orion_seek(struct orion *o, int tid, int pos);
int orion_tell(struct orion *o, int tid);
void orion_ramp(struct orion *o, int tid, float secs, float dst);
void orion_overall_play(struct orion *o);
void orion_overall_pause(struct orion *o);

#endif
