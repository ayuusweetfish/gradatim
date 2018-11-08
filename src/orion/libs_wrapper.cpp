#include "libs_wrapper.h"
#include <soundtouch/SoundTouch.h>
#include <Iir.h>
#include <stdlib.h>

void st_change_tempo(int srate, double delta_pc, int nch, int nsmp, char *inbuf, int *rnsmp, char **outbuf)
{
    // Setup
    soundtouch::SoundTouch st;
    st.setSampleRate(srate);
    st.setChannels(nch);
    st.setTempoChange(delta_pc);
    st.setSetting(SETTING_USE_QUICKSEEK, 0);
    st.setSetting(SETTING_USE_AA_FILTER, 1);

    // Mental preparation... wait, memory setup
    double ratio = st.getInputOutputSampleRatio();
    size_t rcap = (size_t)(nsmp * ratio) + 10;
    size_t rcap_mem = rcap * sizeof(soundtouch::SAMPLETYPE);
    // Use malloc() instead of new, for we'll need to call free() from C
    soundtouch::SAMPLETYPE *obuf = (soundtouch::SAMPLETYPE *)malloc(rcap_mem);

    // Processing
    unsigned int ibufptr = 0, obufptr = 0;
    unsigned int nrecv;
    const int CHUNK_SZ = 6720;
    while (ibufptr < nsmp) {
        unsigned int putsz = (nsmp - ibufptr) / nch;
        if (putsz > CHUNK_SZ) putsz = CHUNK_SZ;
        st.putSamples(((const soundtouch::SAMPLETYPE *)inbuf) + ibufptr, putsz);
        ibufptr += putsz * nch;
        do {
            nrecv = st.receiveSamples(obuf + obufptr, (rcap - obufptr) / nch);
            obufptr += nrecv * nch;
        } while (nrecv != 0);
    }
    st.flush();
    do {
        nrecv = st.receiveSamples(obuf + obufptr, (rcap - obufptr) / nch);
        obufptr += nrecv * nch;
    } while (nrecv != 0);

    *rnsmp = obufptr;
    *outbuf = (char *)obuf;
}

void iir_lowpass(int srate, double cutoff, int nch, int nsmp, char *inbuf, char **outbuf)
{
    short *ibuf = (short *)inbuf,
        *obuf = (short *)malloc(nsmp * sizeof(short));

    Iir::Butterworth::LowPass<4> lp[nch];
    for (int i = 0; i < nch; ++i)
        lp[i].setup(4, srate, cutoff);
    for (int i = 0; i < nsmp; ++i) {
        double o = lp[i % nch].filter((double)ibuf[i] / 0x8000u);
        obuf[i] = (short)(o * 0x8000u);
    }

    *outbuf = (char *)obuf;
}
