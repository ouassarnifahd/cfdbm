#ifndef __HEADER_ALSA_WRAPPER__
#define __HEADER_ALSA_WRAPPER__

// audio capture
void alsa_capture_init();

void alsa_capture_end();

long alsa_capture_read(char* buffer, size_t len);

// audio playback
void alsa_playback_init();

void alsa_playback_end();

long alsa_playback_write(char* buffer, size_t len);

#endif /* end of include guard: __HEADER_ALSA_WRAPPER__ */
