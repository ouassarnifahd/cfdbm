#include "common.h"
#include "alsa_wrapper.h"
#include "buffer_data.h"

char *pdevice = "hw:0,0";       /* playback device */
char *cdevice = "hw:0,0";       /* capture  device */

int rate = 16000;
int channels = 2;

snd_pcm_t *capture_handle, *playback_handle;
snd_pcm_hw_params_t *capture_hw_params, *playback_hw_params;
snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
frame_bytes = (snd_pcm_format_width(format) / 8) * channels;

void capture_init() {
    long err;

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

    if ((err = snd_pcm_hw_params_set_access(capture_handle, capture_hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        error("cannot set access type (%s)", snd_strerror (err));
    }
    debug("capture hw_params access setted");

    if ((err = snd_pcm_hw_params_set_format(capture_handle, capture_hw_params, format)) < 0) {
        error("cannot set sample format (%s)", snd_strerror (err));
    }
    debug("capture hw_params format setted");

    if ((err = snd_pcm_hw_params_set_rate_near(capture_handle, capture_hw_params, &rate, 0)) < 0) {
        error("cannot set sample rate (%s)", snd_strerror (err));
    }
    debug("capture hw_params rate setted");

    if ((err = snd_pcm_hw_params_set_channels(capture_handle, capture_hw_params, channels)) < 0) {
        error("cannot set channel count (%s)", snd_strerror (err));
    }
    debug("capture hw_params channels setted");

    if ((err = snd_pcm_hw_params(capture_handle, capture_hw_params)) < 0) {
        error("cannot set parameters (%s)", snd_strerror (err));
    }
    debug("capture hw_params setted");

    snd_pcm_hw_params_free(capture_hw_params);
    debug("capture hw_params freed");

    if ((err = snd_pcm_prepare(capture_handle)) < 0) {
        error("cannot prepare audio interface for use (%s)", snd_strerror (err));
    }
    debug("audio interface prepared");

}

long capture_read(char* buffer, size_t len) {

    long err;

    do {
        snd_pcm_wait(capture_handle, 1000);
        err = snd_pcm_readi(capture_handle, buffer, len);
        if (err > 0) {
            buffer += err * frame_bytes;
            len    -= err;
        }
        debug("read = %li, len = %li", err, len);
    } while (err >= 1 && len > 0);

    return err;
}

void capture_end() {

    snd_pcm_close(capture_handle);
    debug("audio capture interface closed");

}

void playback_init() {

    long err;

    // int buff_size, loops;
    // seconds  = atoi(argv[3]);

    if (err = snd_pcm_open(&playback_handle, pdevice, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
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
    debug("playback hw_params rate setted");

    if ((err = snd_pcm_hw_params_set_channels(playback_handle, playback_hw_params, channels)) < 0) {
        error("cannot set channel count (%s)", snd_strerror (err));
    }
    debug("playback hw_params channels setted");

    if ((err = snd_pcm_hw_params(playback_handle, playback_hw_params)) < 0) {
        error("cannot set parameters (%s)", snd_strerror (err));
    }
    debug("playback hw_params setted");

    snd_pcm_hw_params_free(playback_hw_params);
    debug("playback hw_params freed");

    if ((err = snd_pcm_prepare(capture_handle)) < 0) {
      error("cannot prepare audio interface for use (%s)",
               snd_strerror (err));
    }
    debug("audio interface prepared");

    /* Resume information */
    // printf("PCM name: '%s'\n", snd_pcm_name(playback_handle));
    //
    // printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(playback_handle)));
    //
    // snd_pcm_hw_params_get_channels(playback_hw_params, &tmp);
    // printf("channels: %i ", tmp);
    //
    // if (tmp == 1)
    //     printf("(mono)\n");
    // else if (tmp == 2)
    //     printf("(stereo)\n");
    //
    // snd_pcm_hw_params_get_rate(playback_hw_params, &tmp, 0);
    // printf("rate: %d bps\n", tmp);
    //
    // printf("seconds: %d\n", seconds);
    //
    // /* Allocate buffer to hold single period */
    // snd_pcm_hw_params_get_period_size(playback_hw_params, &frames, 0);
    //
    // buff_size = frames * channels * 2 /* 2 -> sample size */;
    // snd_pcm_hw_params_get_period_time(playback_hw_params, &tmp, NULL);

    // for (loops = (seconds * 1000000) / tmp; loops > 0; loops--) {
    //
    //     if (pcm = read(0, buff, buff_size) == 0) {
    //         printf("Early end of file.\n");
    //         return 0;
    //     }
    //
    //     if (pcm = snd_pcm_writei(playback_handle, buff, frames) == -EPIPE) {
    //         printf("XRUN.\n");
    //         snd_pcm_prepare(playback_handle);
    //     } else if (pcm < 0) {
    //         printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(pcm));
    //     }
    //
    // }

}

long playback_write(char* buffer, size_t len) {

    long err;

    do {
        snd_pcm_wait(playback_handle, 1000);
        err = snd_pcm_writei(playback_handle, buffer, len);
        if (err > 0) {
            buffer += err * frame_bytes;
            len    -= err;
        }
        debug("write = %li, len = %li", err, len);
    } while (err >= 1 && len > 0);

    return err;

}

void playback_end() {
    snd_pcm_drain(playback_handle);
    snd_pcm_close(playback_handle);
    debug("audio playback interface closed");
}

void gettimestamp(snd_pcm_t *handle, snd_timestamp_t *timestamp) {
    int err;
    snd_pcm_status_t *status;
    snd_pcm_status_alloca(&status);
    if ((err = snd_pcm_status(handle, status)) < 0) {
        error("Stream status error: %s", snd_strerror(err));
    }
    snd_pcm_status_get_trigger_tstamp(status, timestamp);
}

long timediff(snd_timestamp_t t1, snd_timestamp_t t2) {
    signed long l;
    t1.tv_sec -= t2.tv_sec;
    l = (signed long) t1.tv_usec - (signed long) t2.tv_usec;
    if (l < 0) {
        t1.tv_sec--;
        l = 1000000 + l;
        l %= 1000000;
    }
    return (t1.tv_sec * 1000000) + l;
}

long readbuf(snd_pcm_t *handle, char *buf, long len, size_t *frames, size_t *max) {
        long r;
        do {
            r = snd_pcm_readi(handle, buf, len);
            if (r > 0) {
                buf += r * frame_bytes;
                len -= r;
                *frames += r;
                if ((long)*max < r)
                    *max = r;
            }
            debug("r = %li, len = %li", r, len);
        } while (r >= 1 && len > 0);
        // showstat(handle, 0);
        return r;
}

long writebuf(snd_pcm_t *handle, char *buf, long len, size_t *frames) {
        long r;
        while (len > 0) {
                r = snd_pcm_writei(handle, buf, len);
                if (r == -EAGAIN)
                        continue;
                // printf("write = %li", r);
                if (r < 0)
                        return r;
                // showstat(handle, 0);
                buf += r * 4;
                len -= r;
                *frames += r;
        }
        return 0;
}
