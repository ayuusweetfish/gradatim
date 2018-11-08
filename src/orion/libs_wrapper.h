#ifndef _ORION_LIBS_WRAPPER_H
#define _ORION_LIBS_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

// Formats according to SoundTouch compilation (should be 16-bit)
// Outputs in rnsmp and outbuf; outbuf needs to be free()'d
// nch - number of channels
// nsmp - number of samples in inbuf. A stereo sample is considered 2 samples
void st_change_tempo(int srate, double delta_pc, int nch, int nsmp, char *inbuf, int *rnsmp, char **outbuf);

// Assumes 16-bit signed integer
// Outputs in rnsmp and outbuf; outbuf needs to be free()'d
// cutoff - cutoff frequency in Hz
void iir_lowpass(int srate, double cutoff, int nch, int nsmp, char *inbuf, char **outbuf);

#ifdef __cplusplus
}
#endif

#endif
