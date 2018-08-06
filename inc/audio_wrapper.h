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
#define AUDIO_CHANNEL_SAMPLES_COUNT 256
#define AUDIO_SAMPLES_COUNT         (AUDIO_CHANNEL_SAMPLES_COUNT * CHANNELS)

#define FDBM_CHANNEL_SAMPLES_COUNT  512
#define FDBM_SAMPLES_COUNT          (FDBM_CHANNEL_SAMPLES_COUNT * CHANNELS)

#define SAMPLE_BYTES                sizeof(int16_t)
#define FRAME_BYTES                 (CHANNELS * SAMPLE_BYTES)

#define SAMPLES_TO_FRAMES(SAMPLES)  (SAMPLES / CHANNELS)
#define FRAMES_TO_SAMPLES(FRAMES)   (FRAMES * CHANNELS)

#define SAMPLES_TO_RAW(SAMPLES)     (SAMPLES * SAMPLE_BYTES)
#define RAW_TO_SAMPLES(RAW)         (RAW / SAMPLE_BYTES)

#define RAW_FDBM_BUFFER_SIZE        SAMPLES_TO_RAW(FDBM_SAMPLES_COUNT)
#define RAW_AUDIO_BUFFER_SIZE       SAMPLES_TO_RAW(AUDIO_SAMPLES_COUNT)

#define SINT16_MAX                  (((1ull<<16)-1))

// fast audio driver
#ifdef __USE_ALSA__
#include "alsa_wrapper.h"

#define capture_init            alsa_capture_init
#define capture_end             alsa_capture_end
#define capture_read            alsa_capture_read
#define playback_init           alsa_playback_init
#define playback_end            alsa_playback_end
#define playback_write          alsa_playback_write

// offline algorithm processing
#elif __USE_WAV__
#include "wav_wrapper.h"

extern char* filename;

#define capture_init            wav_capture_init
#define capture_end             wav_capture_end
#define capture_read            wav_capture_read
#define playback_init           wav_playback_init
#define playback_end            wav_playback_end
#define playback_write          wav_playback_write

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
