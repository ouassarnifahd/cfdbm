#include "common.h"

// TODO: PulseAUDIO!!! (slower!)
#include <alsa/asoundlib.h>
// see here for more help: http://users.suse.com/~mana/alsa090_howto.html

struct device_t {
    char name[20];
    snd_pcm_t *pcm;
    snd_pcm_stream_t stream;
    snd_pcm_hw_params_t *hw_params;
};

struct param_t {
    snd_pcm_access_t access;
    snd_pcm_format_t format;
    snd_pcm_uframes_t periodsize;
    size_t buffersize;
    size_t periods;
    size_t rate;
    size_t channels;
    size_t bytes_per_frame;
};

struct audio_t {
    device_t device;
    param_t  param;
    buffer_t buffer;
    audioIO_t read;
    audioIO_t write;
};

// typedef struct dep_t dep_t;
char* cdevice = CAPTURE_DEVICE;
char* pdevice = PLAYBACK_DEVICE;

// latency = periodsize * periods / (rate * bytes_per_frame)
#define PERIODS 2

static void read_write_error(char* buffer, size_t frames) {
    error("attempt to read/write using incompatible device!");
}

// static void read_write_error(buffer_t buffer, size_t deps, dep_t *dep) {
//     error("attempt to read/write using incompatible device!");
// }
//
// static void audio_read(buffer_t buffer, size_t deps, dep_t *dep) {
//     alsa_capture_read(buffer.data.raw, buffer.frames);
// }
//
// static void audio_write(buffer_t buffer, size_t deps, dep_t *dep) {
//     alsa_playback_write(buffer.data.raw, buffer.frames);
// }

// alsa_related

// #define AUDIO_INITIALISER(name, stream) { \
//     .device = { \
//         .name = name, .pcm = NULL, \
//         .stream     = stream, \
//         .hw_params  = NULL, \
//     }, \
//     .param = { \
//         .access             = SND_PCM_ACCESS_RW_INTERLEAVED, \
//         .format             = SND_PCM_FORMAT_S16_LE, \
//         .periodsize         = UNINITIALISED, \
//         .buffersize         = UNINITIALISED, \
//         .periods            = PERIODS, \
//         .rate               = RATE, \
//         .channels           = CHANNELS, \
//         .bytes_per_frame    = UNINITIALISED, \
//     }, \
//     .buffer = BUFFER_INITIALISER, \
//     .read   = (stream == SND_PCM_STREAM_CAPTURE) ? alsa_capture_read: read_write_error , \
//     .write  = (stream == SND_PCM_STREAM_PLAYBACK) ? alsa_playback_write: read_write_error, \
// }

audio_t alsa_input = {
    .device = {
        .name = CAPTURE_DEVICE,
        .pcm = NULL,
        .stream = SND_PCM_STREAM_CAPTURE,
        .hw_params = NULL,
    },
    .param = {
        .access = SND_PCM_ACCESS_RW_INTERLEAVED,
        .format = SND_PCM_FORMAT_S16_LE,
        .periodsize = UNINITIALISED,
        .buffersize = UNINITIALISED,
        .periods = PERIODS,
        .rate = RATE,
        .channels = CHANNELS,
        .bytes_per_frame = UNINITIALISED,
    },
    .buffer = {
        .data.raw = NULL,
        .bytes = UNINITIALISED,
        .samples = SAMPLES_COUNT,
        .frames = UNINITIALISED,
        .bytes_per_sample = UNINITIALISED,
        .bytes_per_frame = UNINITIALISED
    },
    .read = alsa_capture_read,
    .write = read_write_error,
};
// AUDIO_INITIALISER(cdevice, SND_PCM_STREAM_CAPTURE);
audio_t alsa_output = {
    .device = {
        .name = PLAYBACK_DEVICE,
        .pcm = NULL,
        .stream = SND_PCM_STREAM_PLAYBACK,
        .hw_params = NULL,
    },
    .param = {
        .access = SND_PCM_ACCESS_RW_INTERLEAVED,
        .format = SND_PCM_FORMAT_S16_LE,
        .periodsize = UNINITIALISED,
        .buffersize = UNINITIALISED,
        .periods = PERIODS,
        .rate = RATE,
        .channels = CHANNELS,
        .bytes_per_frame = UNINITIALISED,
    },
    .buffer = {
        .data.raw = NULL,
        .bytes = UNINITIALISED,
        .samples = SAMPLES_COUNT,
        .frames = UNINITIALISED,
        .bytes_per_sample = UNINITIALISED,
        .bytes_per_frame = UNINITIALISED
    },
    .read = read_write_error,
    .write = alsa_playback_write
};
// AUDIO_INITIALISER(pdevice, SND_PCM_STREAM_PLAYBACK);

audio_t *input = &alsa_input, *output = &alsa_output;

// why?
static size_t alsa_get_frame_bytes(snd_pcm_format_t format, size_t channels) {
    //  snd_pcm_format_width returns bit-width then do / 8 idem (faster) >> 3
    return (((size_t)snd_pcm_format_width(format)) >> 3) * channels;
}

static void alsa_audio_init(audio_t *audio) {

    long err, tmp;

    // setting frame_bytes
    audio->param.bytes_per_frame = alsa_get_frame_bytes(audio->param.format, audio->param.channels);
    // preparing buffersize and periodsize from SAMPLES_COUNT = 512!!
    audio->param.periodsize = SAMPLES_COUNT;
    audio->param.buffersize = audio->param.periodsize * audio->param.periods;

    // allocate audio software buffer
    audio->buffer.bytes_per_frame  = audio->param.bytes_per_frame;
    audio->buffer.bytes_per_sample = audio->param.bytes_per_frame / audio->param.channels;
    audio->buffer.samples = SAMPLES_COUNT;
    audio->buffer.bytes = audio->buffer.bytes_per_sample * audio->buffer.samples;
    // audio->buffer.data.raw  = aligned_alloc(MEM_ALIGN, audio->buffer.bytes);
    // alloc_check(audio->buffer.data.raw);
    debug("ALSA library version: %s", SND_LIB_VERSION_STR);

    if ((err = snd_pcm_open(&audio->device.pcm, audio->device.name, audio->device.stream, 0)) < 0) {
        error("cannot open audio device %s (%s)", audio->device.name, snd_strerror (err));
    }
    debug("audio interface opened");

    if ((err = snd_pcm_hw_params_malloc(&audio->device.hw_params)) < 0) {
        error("cannot allocate hardware parameter structure (%s)", snd_strerror (err));
    }
    debug("hw_params allocated");

    if ((err = snd_pcm_hw_params_any(audio->device.pcm, audio->device.hw_params)) < 0) {
        error("cannot initialize hardware parameter structure (%s)", snd_strerror (err));
    }
    debug("hw_params initialized");

    if ((err = snd_pcm_hw_params_set_access(audio->device.pcm, audio->device.hw_params, audio->param.access)) < 0) {
        error("cannot set access type (%s)", snd_strerror(err));
    }
    debug("hw_params access setted");

    if ((err = snd_pcm_hw_params_set_format(audio->device.pcm, audio->device.hw_params, audio->param.format)) < 0) {
        error("cannot set sample format (%s)", snd_strerror(err));
    }
    debug("hw_params format setted");

    if ((err = snd_pcm_hw_params_set_rate(audio->device.pcm, audio->device.hw_params, audio->param.rate, 0)) < 0) {
        error("cannot set sample rate (%s)", snd_strerror(err));
    }
    snd_pcm_hw_params_get_rate(audio->device.hw_params, &tmp, 0);
    debug("hw_params rate set to %u", tmp);

    if ((err = snd_pcm_hw_params_set_channels(audio->device.pcm, audio->device.hw_params, audio->param.channels)) < 0) {
        error("cannot set channel count (%s)", snd_strerror(err));
    }
    snd_pcm_hw_params_get_channels(audio->device.hw_params, &tmp);
    debug("hw_params channels set to %u", tmp);

    if ((err = snd_pcm_hw_params_set_periods_near(audio->device.pcm, audio->device.hw_params, &audio->param.periods, NULL)) < 0) {
        error("cannot set periods (%s)", snd_strerror (err));
    }
    debug("hw_params periods setted");

    if ((err = snd_pcm_hw_params_set_buffer_size_near(audio->device.pcm, audio->device.hw_params, &audio->param.buffersize)) < 0) {
        error("cannot set buffer size (%s)", snd_strerror (err));
    }
    debug("hw_params buffer size setted");

    if ((err = snd_pcm_hw_params(audio->device.pcm, audio->device.hw_params)) < 0) {
        error("cannot set parameters (%s)", snd_strerror(err));
    }
    debug("hw_params setted");

    snd_pcm_hw_params_free(audio->device.hw_params);
    debug("hw_params freed");

    if ((err = snd_pcm_prepare(audio->device.pcm)) < 0) {
        error("cannot prepare audio interface for use (%s)", snd_strerror (err));
    }
    debug("audio interface prepared");

}

void alsa_capture_init() {
    debug("audio capture init");
    alsa_audio_init(&alsa_input);
}

void alsa_capture_end() {
    snd_pcm_close(alsa_input.device.pcm);
    debug("audio capture interface closed");
    free(alsa_input.buffer.data.raw);
}

long alsa_capture_read(char* buffer, size_t frames) {

    long read = 0, total = frames;
    // nice try! (const char* buffer might be better!)
    char* start_pointer = buffer;

    while (frames > 0) {
        read = snd_pcm_readi(alsa_input.device.pcm, buffer, frames);

        if (read == -EAGAIN || (read >= 0 && read < frames)) {
            snd_pcm_wait(alsa_input.device.pcm, 10);
        } else if (read == -EPIPE) {
            if(snd_pcm_prepare(alsa_input.device.pcm) < 0)
                return -1;
        } else if (read == -ESTRPIPE) {
            int err;
            while ((err = snd_pcm_resume(alsa_input.device.pcm)) == -EAGAIN)
                sleep_ms(1);   /* wait until suspend flag is released */
            if (err < 0) {
                if (snd_pcm_prepare(alsa_input.device.pcm) < 0) {
                    return -1;
                }
            }
        } else if (read < 0) {
            return -1;
        }
        if (read > 0) {
            frames -= read;
            buffer += read * alsa_input.param.bytes_per_frame;
        }
        #ifdef __DEBUGED__
        debug("Frames to read = %d: Frames allready read = %d", frames, read);
		debug("Read Success");
        #endif
    }
    // dammmit!
    // memcpy(buffer, start_pointer, total * alsa_input.param.bytes_per_frame);
    buffer = start_pointer;
    return total;
}

void alsa_playback_init() {
    debug("audio playback init");
    alsa_audio_init(&alsa_output);
}

void alsa_playback_end() {
    snd_pcm_drain(alsa_output.device.pcm);
    snd_pcm_close(alsa_output.device.pcm);
    debug("audio playback interface closed");
    free(alsa_output.buffer.data.raw);
}

long alsa_playback_write(char* buffer, size_t frames) {

    int total = 0, written = 0;
    alsa_output.buffer.data.raw = buffer;
    // memcpy(alsa_output.buffer.data.raw, buffer, alsa_output.buffer.frames);
    while (frames > 0) {
        written = snd_pcm_writei(alsa_output.device.pcm, alsa_output.buffer.data.raw, frames);

        if (written == -EAGAIN || (written >= 0 && written < frames)) {
            snd_pcm_wait(alsa_output.device.pcm, 10);
        } else if (written == -EPIPE) {
            if(snd_pcm_prepare(alsa_output.device.pcm) < 0)
                return -1;
        } else if (written == -ESTRPIPE) {
            int err;
            while ((err = snd_pcm_resume(alsa_output.device.pcm)) == -EAGAIN)
                sleep_ms(1);   /* wait until suspend flag is released */
            if (err < 0) {
                if (snd_pcm_prepare(alsa_output.device.pcm) < 0) {
                    return -1;
                }
            }
        } else if (written < 0) {
            return -1;
        }

        if (written > 0) {
            total += written;
            frames -= written;
            alsa_output.buffer.data.raw += written * alsa_output.param.bytes_per_frame;
        }
        #ifdef __DEBUGED__
        debug("Write Success");
        #endif
    }

    return total;
}
