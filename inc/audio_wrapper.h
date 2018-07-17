#ifndef __HEADER_AUDIO_WRAPPER__
#define __HEADER_AUDIO_WRAPPER__

#define PLAYBACK_DEVICE "plughw:0,0"
#define CAPTURE_DEVICE  "plughw:0,0"
#define CHANNELS 2

#define FDBM_RATE 16000
#define CHANNEL_SAMPLES_COUNT (512)
#define SAMPLES_COUNT (CHANNEL_SAMPLES_COUNT * CHANNELS)

// FRAME_BYTES = CHANNELS * sizeof(int16_t)
#define FRAME_BYTES 4
#define SAMPLES_TO_RAW(SAMPLES) (SAMPLES * FRAME_BYTES)
#define RAW_TO_SAMPLES(RAW)     (RAW / FRAME_BYTES)

#define RAW_FDBM_BUFFER_SIZE (SAMPLES_TO_RAW(SAMPLES_COUNT))
#define RAW_AUDIO_BUFFER_SIZE (RAW_FDBM_BUFFER_SIZE)

#define SINT16_MAX (((1ull<<16)-1))

// fast audio driver
#ifdef __USE_ALSA__
#include "alsa_wrapper.h"

#define capture_init    alsa_capture_init
#define capture_end     alsa_capture_end
#define capture_read    alsa_capture_read
#define playback_init   alsa_playback_init
#define playback_end    alsa_playback_end
#define playback_write  alsa_playback_write

// easier audio driver
#else // __USE_PULSEAUDIO__
#include "pa_wrapper.h"

#define capture_init    pa_capture_init
#define capture_end     pa_capture_end
#define capture_read    pa_capture_read
#define playback_init   pa_playback_init
#define playback_end    pa_playback_end
#define playback_write  pa_playback_write

#endif

#endif /* end of include guard: __HEADER_AUDIO_WRAPPER__ */
