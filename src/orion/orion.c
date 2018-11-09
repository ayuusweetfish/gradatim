#include "orion.h"

#include <vorbis/vorbisfile.h>
#include <SDL2/SDL.h>
#include <portaudio.h>

#include <stdlib.h>
#include <string.h>

#define IS_BIGENDIAN    (!*(unsigned char *)&(uint16_t){1})
#define IS_SIGNED(__t)  ((__t)0 - 1 < 0)

struct orion orion_create(int srate, int nch)
{
    struct orion ret = { 0 };
    ret.srate = srate;
    ret.nch = nch;
    return ret;
}

void orion_drop(struct orion *o)
{
    SDL_AtomicLock(&o->lock);
    int i;
    for (i = 0; i < ORION_NUM_TRACKS; ++i)
        if (o->track[i].pcm != NULL)
            free(o->track[i].pcm);
    SDL_AtomicUnlock(&o->lock);
}

const char *orion_load_ogg(struct orion *o, int tid, const char *path)
{
    /* Load the entire file with Ogg Vorbis */
    OggVorbis_File vf;
    int ret = ov_fopen(path, &vf);
    if (ret < 0) switch (ret) {
        case OV_EREAD: return "Error happened during file read";
        case OV_ENOTVORBIS:
        case OV_EBADHEADER: return "Not a valid Ogg Vorbis file";
        case OV_EVERSION: return "Vorbis version mismatch";
        case OV_EFAULT:
        default: return "Internal error in Vorbisfile library";
    }
    double secs = (double)ov_time_total(&vf, -1);
    int nch = ov_info(&vf, -1)->channels;
    int srate = ov_info(&vf, -1)->rate;
    int buf_sz = sizeof(orion_smp) * secs * srate * nch + 8;
    char *buf = (char *)malloc(buf_sz);
    int buf_ptr = 0, nread;
    while ((nread = ov_read(&vf, buf + buf_ptr, buf_sz,
        IS_BIGENDIAN, sizeof(orion_smp), IS_SIGNED(orion_smp), NULL)) != 0)
    {
        buf_ptr += nread;
        buf_sz -= nread;
    }
    ov_clear(&vf);

    /* Store information into the track struct */
    SDL_AtomicLock(&o->lock);
    memset(&o->track[tid], 0, sizeof o->track[tid]);
    /* In order to minimize the work done when holding the lock,
     * we store the address and defer the memory deallocation */
    orion_smp *free_ptr = o->track[tid].pcm;
    o->track[tid].nch = nch;
    o->track[tid].len = buf_ptr / nch / sizeof(orion_smp);
    o->track[tid].pcm = (orion_smp *)buf;
    o->track[tid].state = ORION_STOPPED;
    SDL_AtomicUnlock(&o->lock);
    if (free_ptr != NULL) free(free_ptr);

    /* Finish with no errors */
    return NULL;
}

void orion_apply_lowpass(struct orion *o, int tid, int did, double cutoff)
{
    char *buf;
    iir_lowpass(
        o->srate, cutoff, o->track[tid].nch,
        o->track[tid].len * o->track[tid].nch,
        (char *)o->track[tid].pcm, &buf);

    SDL_AtomicLock(&o->lock);
    orion_smp *free_ptr = o->track[did].pcm;
    o->track[did] = o->track[tid];
    o->track[did].pcm = (orion_smp *)buf;
    o->track[did].state = ORION_STOPPED;
    SDL_AtomicUnlock(&o->lock);
    if (free_ptr != NULL) free(free_ptr);
}

void orion_apply_stretch(struct orion *o, int tid, int did, double delta_pc)
{
    int len;
    char *buf;
    st_change_tempo(
        o->srate, delta_pc, o->track[tid].nch,
        o->track[tid].len * o->track[tid].nch,
        (char *)o->track[tid].pcm, &len, &buf);

    SDL_AtomicLock(&o->lock);
    orion_smp *free_ptr = o->track[did].pcm;
    o->track[did] = o->track[tid];
    o->track[did].len = len / o->track[tid].nch;
    o->track[did].pcm = (orion_smp *)buf;
    o->track[did].state = ORION_STOPPED;
    SDL_AtomicUnlock(&o->lock);
    if (free_ptr != NULL) free(free_ptr);
}

void orion_play_once(struct orion *o, int tid)
{
    SDL_AtomicLock(&o->lock);
    if (o->track[tid].state != ORION_STOPPED) return;
    o->track[tid].play_pos = 0;
    o->track[tid].volume = 1;
    o->track[tid].loop_start = -1;
    o->track[tid].loop_end = o->track[tid].len;
    o->track[tid].ramp_slope = 0;
    o->track[tid].state = ORION_ONCE;
    SDL_AtomicUnlock(&o->lock);
}

void orion_play_loop(struct orion *o, int tid, int intro_pos, int start_pos, int end_pos)
{
    SDL_AtomicLock(&o->lock);
    if (o->track[tid].state != ORION_STOPPED) return;
    o->track[tid].play_pos = intro_pos;
    o->track[tid].volume = 1;
    int l = o->track[tid].len;
    start_pos = ((start_pos % l) + l) % l;
    end_pos = ((end_pos % l) + l) % l;
    o->track[tid].loop_start = start_pos;
    o->track[tid].loop_end = end_pos;
    o->track[tid].ramp_slope = 0;
    o->track[tid].state = ORION_LOOP;
    SDL_AtomicUnlock(&o->lock);
}

void orion_pause(struct orion *o, int tid)
{
    SDL_AtomicLock(&o->lock);
    if (o->track[tid].state <= ORION_STOPPED) return;
    o->track[tid].state = ORION_STOPPED;
    SDL_AtomicUnlock(&o->lock);
}

void orion_resume(struct orion *o, int tid)
{
    SDL_AtomicLock(&o->lock);
    if (o->track[tid].state != ORION_STOPPED) return;
    o->track[tid].state =
        (o->track[tid].loop_start == -1) ? ORION_ONCE : ORION_LOOP;
    SDL_AtomicUnlock(&o->lock);
}

void orion_ramp(struct orion *o, int tid, float secs, float dst)
{
    SDL_AtomicLock(&o->lock);
    if (o->track[tid].state != ORION_STOPPED) return;
    int smps = secs * o->srate;
    float cur_vol = o->track[tid].volume;
    o->track[tid].ramp_end = smps;
    o->track[tid].ramp_slope = (double)(dst - cur_vol) / smps;
    SDL_AtomicUnlock(&o->lock);
}

/* The callback invoked by PortAudio.
 * Fills the buffer according to the pointers in the struct. */
static int _orion_portaudio_callback(
    const void *ibuf, void *_obuf, unsigned long nframes,
    const PaStreamCallbackTimeInfo *time,
    PaStreamCallbackFlags flags, void *_o)
{
    struct orion *o = (struct orion *)_o;
    orion_smp *obuf = (orion_smp *)_obuf;
    SDL_AtomicLock(&o->lock);
    int nch = o->nch;
    SDL_AtomicUnlock(&o->lock);
    int i, j;
    for (i = 0; i < nframes; ++i)
        for (j = 0; j < nch; ++j) {
            *obuf++ = (((j ^ i) & 1) ? i * 100 : 0);
        }
    return 0;
}

/* The subroutine that (re-)initializes PortAudio and starts playback. */
static int _orion_playback_routine(void *_o)
{
    struct orion *o = (struct orion *)_o;
    PaError pa_err;

    pa_err = Pa_Initialize();
    if (pa_err != paNoError)
        return 1;   /* Do not goto err as Pa_Terminate() needn't be called */

    /* Retrieve parameters */
    SDL_AtomicLock(&o->lock);
    int srate = o->srate, nch = o->nch;
    SDL_AtomicUnlock(&o->lock);

    /* PortAudio setup */
    PaStreamParameters param;
    PaStream *stream;
    param.device = Pa_GetDefaultOutputDevice();
    if (param.device == paNoDevice) goto err;
    param.channelCount = nch;
    param.sampleFormat = paInt16;   /* FIXME: Keep in sync with orion_smp */
    param.suggestedLatency =
        Pa_GetDeviceInfo(param.device)->defaultLowOutputLatency;
    param.hostApiSpecificStreamInfo = NULL;

    /* TODO: Make framesPerBuffer (64) configurable */
    pa_err = Pa_OpenStream(
        &stream, NULL, &param, srate, 64,
        paNoFlag, _orion_portaudio_callback, _o);
    if (pa_err != paNoError) goto err;
    pa_err = Pa_StartStream(stream);
    if (pa_err != paNoError) goto err;

    /* Enter the loop */
    static const int SLEEP_INTV = 10;
    unsigned char running = 1;
    while (running) {
        Pa_Sleep(SLEEP_INTV);
        SDL_AtomicLock(&o->lock);
        running = o->is_playing;
        SDL_AtomicUnlock(&o->lock);
    }

    Pa_Terminate();
    return 0;

err:
    Pa_Terminate();
    /* XXX: Do something else? */
    return 0;
}

void orion_overall_play(struct orion *o)
{
    SDL_AtomicLock(&o->lock);
    if (o->is_playing) goto exit;

    /* Create the thread; it will start running right away */
    SDL_Thread *th = SDL_CreateThread(
        _orion_playback_routine, "Orion playback", o
    );
    if (th == NULL) goto exit;  /* XXX: Inform the caller? */

    /* Update the struct */
    o->is_playing = 1;
    o->playback_thread = th;

exit:
    SDL_AtomicUnlock(&o->lock);
}

void orion_overall_pause(struct orion *o)
{
    SDL_AtomicLock(&o->lock);
    if (!o->is_playing) goto exit;
    o->is_playing = 0;
exit:
    SDL_AtomicUnlock(&o->lock);
    /* This will cause the thread to exit automatically */
}
