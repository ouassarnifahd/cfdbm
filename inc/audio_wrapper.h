#ifndef __HEADER_AUDIO__
#define __HEADER_AUDIO__

// PUBLIC DEFINES
#define PLAYBACK_DEVICE         "plughw:0,0"
#define CAPTURE_DEVICE          "plughw:2,0"
#define RATE                    16000
#define CHANNELS                2
#define UNINITIALISED           0

// remove me later
#define MEM_ALIGN 32

// PRIVATE DEFINES
#define CHANNEL_SAMPLES_COUNT   512
#define SAMPLES_COUNT           (CHANNEL_SAMPLES_COUNT * CHANNELS)

// FRAME_BYTES = CHANNELS * sizeof(int16_t)
#define SAMPLE_BYTES            2
#define FRAME_BYTES             (CHANNELS * SAMPLE_BYTES)

#define SAMPLES_TO_FRAMES(SAMPLES) (SAMPLES / CHANNELS)
#define FRAMES_TO_SAMPLES(FRAMES)  (FRAMES * CHANNELS)

#define SAMPLES_TO_RAW(SAMPLES) (SAMPLES * SAMPLE_BYTES)
#define RAW_TO_SAMPLES(RAW)     (RAW / SAMPLE_BYTES)

#define RAW_FDBM_BUFFER_SIZE    (SAMPLES_COUNT * SAMPLE_BYTES)
#define RAW_AUDIO_BUFFER_SIZE   (RAW_FDBM_BUFFER_SIZE)

#define SINT16_MAX              (((1ull<<16)-1))

// fast audio driver
#ifdef __USE_ALSA__
#include "alsa_wrapper.h"

#define capture_init            alsa_capture_init
#define capture_end             alsa_capture_end
#define capture_read            alsa_capture_read
#define playback_init           alsa_playback_init
#define playback_end            alsa_playback_end
#define playback_write          alsa_playback_write

// easier audio driver (but I cannot customize it YET)
#else // __USE_PULSEAUDIO__
#include "pa_wrapper.h"

#define capture_init            pa_capture_init
#define capture_end             pa_capture_end
#define capture_read            pa_capture_read
#define playback_init           pa_playback_init
#define playback_end            pa_playback_end
#define playback_write          pa_playback_write

#endif // __USE_ALSA__

typedef struct device_t device_t;
typedef struct param_t param_t;

struct buffer_t {
    union { char* raw; int16_t* samples; } data;
    size_t bytes;
    size_t samples;
    size_t frames;
    size_t bytes_per_sample;
    size_t bytes_per_frame;
};
typedef struct buffer_t buffer_t;

typedef long (*audioIO_t)(char*, size_t);

// typedef void (*algo_t)(buffer_t, size_t, dep_t*);

typedef struct audio_t audio_t;

extern audio_t *input;
extern audio_t *output;

#endif /* end of include guard: __HEADER_AUDIO__ */
