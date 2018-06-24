#ifndef __HEADER_ALSA_WRAPPER__
#define __HEADER_ALSA_WRAPPER__

#include <alsa/asoundlib.h>

int buf_init(snd_pcm_t *phandle, snd_pcm_t *chandle, char *buffer, int *latency);

void buf_end(snd_pcm_t *phandle, snd_pcm_t *chandle);

long readbuf(snd_pcm_t *handle, char *buf, long len, size_t *frames, size_t *max);

long writebuf(snd_pcm_t *handle, char *buf, long len, size_t *frames);

#endif /* end of include guard: __HEADER_ALSA_WRAPPER__ */
