#include "common.h"

// TODO: this isnt the way see: PulseAUDIO!!!
#include <alsa/asoundlib.h>

char *pdevice = PLAYBACK_DEVICE;       /* playback device */
char *cdevice = CAPTURE_DEVICE;       /* capture  device */

unsigned int rate = FDBM_RATE;
unsigned int channels = CHANNELS;
unsigned int buffer_time = 500000;
int dir = 1;
int frame_bytes;

snd_pcm_t *capture_handle, *playback_handle;
snd_pcm_hw_params_t *capture_hw_params, *playback_hw_params;
snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

int alsa_get_frame_bytes() {
    return (snd_pcm_format_width(format) / 8) * channels;
}

void alsa_capture_init() {
    long err, tmp;

    frame_bytes = alsa_get_frame_bytes();

    if ((err = snd_pcm_open(&capture_handle, cdevice, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        error("cannot open audio device %s (%s)", cdevice, snd_strerror (err));
    }
    debug("audio capture interface opened");

    if ((err = snd_pcm_hw_params_malloc(&capture_hw_params)) < 0) {
        error("cannot allocate hardware parameter structure (%s)", snd_strerror (err));
    }
    debug("capture hw_params allocated");

    if ((err = snd_pcm_hw_params_any(capture_handle, capture_hw_params)) < 0) {
        error("cannot initialize hardware parameter structure (%s)", snd_strerror (err));
    }
    debug("capture hw_params initialized");

    // if ((err = snd_pcm_hw_params_set_rate_resample(capture_handle, capture_hw_params, 1)) < 0) {
    //     error("cannot set hardware resample rate (%s)", snd_strerror(err));
    // }
    // debug("capture hw_params resample setted");

    if ((err = snd_pcm_hw_params_set_access(capture_handle, capture_hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        error("cannot set access type (%s)", snd_strerror(err));
    }
    debug("capture hw_params access setted");

    if ((err = snd_pcm_hw_params_set_format(capture_handle, capture_hw_params, format)) < 0) {
        error("cannot set sample format (%s)", snd_strerror(err));
    }
    debug("capture hw_params format setted");

    if ((err = snd_pcm_hw_params_set_rate(capture_handle, capture_hw_params, rate, 0)) < 0) {
        error("cannot set sample rate (%s)", snd_strerror(err));
    }
    snd_pcm_hw_params_get_rate(capture_hw_params, &tmp, 0);
    debug("capture hw_params rate setted to %u", tmp);

    if ((err = snd_pcm_hw_params_set_channels(capture_handle, capture_hw_params, channels)) < 0) {
        error("cannot set channel count (%s)", snd_strerror(err));
    }
    snd_pcm_hw_params_get_channels(capture_hw_params, &tmp);
    debug("capture hw_params channels setted to %u", tmp);

    if ((err = snd_pcm_hw_params_set_buffer_time_near(capture_handle, capture_hw_params, &buffer_time, &dir)) < 0) {
        error("cannot set time buffer (%s)", snd_strerror (err));
    }
    debug("playback hw_params time buffer setted to %u us", buffer_time);
    buffer_time = 500000;

    if ((err = snd_pcm_hw_params(capture_handle, capture_hw_params)) < 0) {
        error("cannot set parameters (%s)", snd_strerror(err));
    }
    debug("capture hw_params setted");

    snd_pcm_hw_params_free(capture_hw_params);
    debug("capture hw_params freed");

    if ((err = snd_pcm_prepare(capture_handle)) < 0) {
        error("cannot prepare capture audio interface for use (%s)", snd_strerror (err));
    }
    debug("capture audio interface prepared");

}

long alsa_capture_read(char* buffer, size_t len) {

    int read;

    int frames = len / frame_bytes;

    while (frames > 0) {
        read = snd_pcm_readi(capture_handle, buffer, frames);

        if (read == -EAGAIN || (read >= 0 && read < frames)) {
            snd_pcm_wait(capture_handle, 10);
        } else if (read == -EPIPE) {
            if(snd_pcm_prepare(capture_handle) < 0)
                return -1;
        } else if (read == -ESTRPIPE) {
            int err;
            while ((err = snd_pcm_resume(capture_handle)) == -EAGAIN)
                sleep_ms(1);   /* wait until suspend flag is released */
            if (err < 0) {
                if (snd_pcm_prepare(capture_handle) < 0) {
                    return -1;
                }
            }
        } else if (read < 0) {
            return -1;
        }
        if (read > 0) {
            frames -= read;
            buffer += read * frame_bytes;
        }
        #ifdef __DEBUGED__
        debug("Frames to read = %d: Frames allready read = %d", frames, read);
		debug("Read Success");
        #endif
    }

    return len;
}

void alsa_capture_end() {

    snd_pcm_close(capture_handle);
    debug("audio capture interface closed");

}

void alsa_playback_init() {

    long err, tmp;

    // int buff_size, loops;
    // seconds  = atoi(argv[3]);

    if ((err = snd_pcm_open(&playback_handle, pdevice, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        error("cannot open audio device %s (%s)", pdevice, snd_strerror(err));
    }
    debug("audio playback interface opened");

    if ((err = snd_pcm_hw_params_malloc(&playback_hw_params)) < 0) {
        error("cannot allocate hardware parameter structure (%s)", snd_strerror (err));
    }
    debug("playback hw_params allocated");

    if ((err = snd_pcm_hw_params_any(playback_handle, playback_hw_params)) < 0) {
        error("cannot initialize hardware parameter structure (%s)", snd_strerror (err));
    }
    debug("playback hw_params initialized");

    // if ((err = snd_pcm_hw_params_set_rate_resample(playback_handle, playback_hw_params, 1)) < 0) {
    //     error("cannot set hardware resample rate (%s)", snd_strerror(err));
    // }
    // debug("playback hw_params resample setted");

    if ((err = snd_pcm_hw_params_set_access(playback_handle, playback_hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        error("cannot set access type (%s)", snd_strerror (err));
    }
    debug("playback hw_params access setted");

    if ((err = snd_pcm_hw_params_set_format(playback_handle, playback_hw_params, format)) < 0) {
        error("cannot set sample format (%s)", snd_strerror(err));
    }
    debug("playback hw_params format setted");

    if ((err = snd_pcm_hw_params_set_rate_near(playback_handle, playback_hw_params, &rate, 0)) < 0) {
        error("cannot set sample rate (%s)", snd_strerror (err));
    }
    snd_pcm_hw_params_get_rate(playback_hw_params, &tmp, 0);
    debug("playback hw_params rate setted to %u", tmp);

    if ((err = snd_pcm_hw_params_set_channels(playback_handle, playback_hw_params, channels)) < 0) {
        error("cannot set channel count (%s)", snd_strerror (err));
    }
    snd_pcm_hw_params_get_channels(playback_hw_params, &tmp);
    debug("playback hw_params channels setted to %i", tmp);

    if ((err = snd_pcm_hw_params_set_buffer_time_near(playback_handle, playback_hw_params, &buffer_time, &dir)) < 0) {
        error("cannot set time buffer (%s)", snd_strerror (err));
    }
    debug("playback hw_params time buffer setted to %u", buffer_time);

    if ((err = snd_pcm_hw_params(playback_handle, playback_hw_params)) < 0) {
        error("cannot set parameters (%s)", snd_strerror (err));
    }
    debug("playback hw_params setted");

    snd_pcm_hw_params_free(playback_hw_params);
    debug("playback hw_params freed");

    if ((err = snd_pcm_prepare(playback_handle)) < 0) {
        error("cannot prepare playback audio interface for use (%s)", snd_strerror (err));
    }
    debug("playback audio interface prepared");

}

void alsa_playback_end() {
    snd_pcm_drain(playback_handle);
    snd_pcm_close(playback_handle);
    debug("audio playback interface closed");
}

long alsa_playback_write(char* buffer, size_t len) {

    int frames = len / frame_bytes;
    int total = 0;
    while (frames > 0) {
        int written = snd_pcm_writei(playback_handle, buffer, frames);

        if (written == -EAGAIN || (written >= 0 && written < frames)) {
            snd_pcm_wait(playback_handle, 10);
        } else if (written == -EPIPE) {
            if(snd_pcm_prepare(playback_handle) < 0)
                return -1;
        } else if (written == -ESTRPIPE) {
            int err;
            while ((err = snd_pcm_resume(playback_handle)) == -EAGAIN)
                sleep_ms(1);   /* wait until suspend flag is released */
            if (err < 0) {
                if (snd_pcm_prepare(playback_handle) < 0) {
                    return -1;
                }
            }
        } else if (written < 0) {
            return -1;
        }

        if (written > 0) {
            total += written;
            frames -= written;
            buffer += written * frame_bytes;
        }
        #ifdef __DEBUGED__
        debug("Write Success");
        #endif
    }

    return total * frame_bytes;
}
