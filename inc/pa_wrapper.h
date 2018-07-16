#ifndef __HEADER_PA_WRAPPER__
#define __HEADER_PA_WRAPPER__

// audio capture
void pa_capture_init();

void pa_capture_end();

long pa_capture_read(char* buffer, size_t len);

// audio playback
void pa_playback_init();

void pa_playback_end();

long pa_playback_write(char* buffer, size_t len);

#endif /* end of include guard: __HEADER_PA_WRAPPER__ */
