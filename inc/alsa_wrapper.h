#ifndef __HEADER_ALSA_WRAPPER__
#define __HEADER_ALSA_WRAPPER__

#include <alsa/asoundlib.h>

// time spent in capture = n / rate = 32 ms
#define SAMPLES_COUNT (512)
#define CHANNEL_SAMPLES_COUNT (SAMPLES_COUNT/2)

int get_frame_bytes();
#define RAW_BUFFER_SIZE (SAMPLES_COUNT * get_frame_bytes())

#define SINT16_MAX (((1ull<<15)-1))

// audio capture
void capture_init();

void capture_end();

long capture_read(char* buffer, size_t len);

// audio playback
void playback_init();

void playback_end();

long playback_write(char* buffer, size_t len);

// link
void capture_playback_link(char* buffer, size_t len);
void capture_playback_unlink();


// old API
// long readbuf(snd_pcm_t *handle, char *buf, long len, size_t *frames, size_t *max);
//
// long writebuf(snd_pcm_t *handle, char *buf, long len, size_t *frames);

#endif /* end of include guard: __HEADER_ALSA_WRAPPER__ */
