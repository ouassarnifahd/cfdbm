#ifndef __HEADER_ALSA_WRAPPER__
#define __HEADER_ALSA_WRAPPER__

// TODO: this isnt the way see: PulseAUDIO!!!
#include <alsa/asoundlib.h>

#define DEFAULT_RATE 16000
// time spent in capture = n / rate = 32 ms
#define CHANNEL_SAMPLES_COUNT (512)
#define SAMPLES_COUNT (SAMPLES_COUNT * 2)

int get_frame_bytes();
#define SAMPLES_TO_RAW(SAMPLES) (SAMPLES * get_frame_bytes())
#define RAW_TO_SAMPLES(RAW)     (RAW / get_frame_bytes())

#define RAW_FDBM_BUFFER_SIZE (SAMPLES_TO_RAW(SAMPLES_COUNT))
#define RAW_ALSA_BUFFER_SIZE (RAW_FDBM_BUFFER_SIZE / 2)

#define SINT16_MAX (((1ull<<16)-1))

// audio capture
void capture_init();

void capture_end();

long capture_read(char* buffer, size_t len);

// audio playback
void playback_init();

void playback_end();

long playback_write(char* buffer, size_t len);

// old API
// long readbuf(snd_pcm_t *handle, char *buf, long len, size_t *frames, size_t *max);
// long writebuf(snd_pcm_t *handle, char *buf, long len, size_t *frames);
//
// link
// void capture_playback_link(char* buffer, size_t len);
// void capture_playback_unlink();

#endif /* end of include guard: __HEADER_ALSA_WRAPPER__ */
