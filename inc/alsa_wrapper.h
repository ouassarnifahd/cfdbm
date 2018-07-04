#ifndef __HEADER_ALSA_WRAPPER__
#define __HEADER_ALSA_WRAPPER__

#include <alsa/asoundlib.h>

int get_frame_bytes();

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
