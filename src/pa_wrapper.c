#include "common.h"
#include "audio_wrapper.h"

#ifndef __arm__

#include <pulse/simple.h>
#include <pulse/error.h>

static const pa_sample_spec ss = {
    .format = PA_SAMPLE_S16LE,
    .rate = RATE,
    .channels = CHANNELS
};

pa_simple *capture_stream = NULL, *playback_stream = NULL;

// audio capture
void pa_capture_init() {
    /* Create the recording stream */
    int error;
    pa_channel_map m;
    pa_channel_map* stereo_map = pa_channel_map_init_stereo(&m);

    if (!(capture_stream = pa_simple_new(
            NULL,               /* default server */
            "CFDBM",            /* client name */
            PA_STREAM_RECORD,   /* recording or playback */
            NULL/*CAPTURE_DEVICE*/,     /* device */
            "stereo recording", /* stream name */
            &ss,                /* sample spec */
            stereo_map,         /* channel map */
            NULL,               /* Buffering attributes */
            &error))) {
        warning("pa_simple_new() failed: %s", pa_strerror(error));
        pa_capture_end();
    }
}

void pa_capture_end() {
    if (capture_stream)
        pa_simple_free(capture_stream);
}

long pa_capture_read(char* buffer, size_t len) {
    int error;
    /* Record some data ... */
    if (pa_simple_read(capture_stream, buffer, len, &error) < 0) {
        warning("pa_simple_read() failed: %s", pa_strerror(error));
        pa_capture_end();
    }
}

// audio playback
void pa_playback_init() {
    /* Create a new playback stream */
    int error;
    pa_channel_map m;
    pa_channel_map* stereo_map = pa_channel_map_init_stereo(&m);

    if (!(playback_stream = pa_simple_new(
            NULL,               /* default server */
            "CFDBM",            /* client name */
            PA_STREAM_PLAYBACK, /* recording or playback */
            NULL/*PLAYBACK_DEVICE*/,    /* device */
            "stereo playback",  /* stream name */
            &ss,                /* sample spec */
            stereo_map,         /* channel map */
            NULL,               /* Buffering attributes */
            &error))) {
        warning("pa_simple_new() failed: %s", pa_strerror(error));
        pa_playback_end();
    }
}

void pa_playback_end() {
    int error;
    /* Make sure that every single sample was played */
    if (pa_simple_drain(playback_stream, &error) < 0) {
        warning("pa_simple_drain() failed: %s", pa_strerror(error));
    }
    if (playback_stream)
        pa_simple_free(playback_stream);
}

long pa_playback_write(char* buffer, size_t len) {
    int error;
    /* ... and play it */
    if (pa_simple_write(playback_stream, buffer, len, &error) < 0) {
        warning("pa_simple_write() failed: %s", pa_strerror(error));
        pa_playback_end();
    }
    /* Make sure that every single sample was played */
    if (pa_simple_drain(playback_stream, &error) < 0) {
        warning("pa_simple_drain() failed: %s\n", pa_strerror(error));
        pa_playback_end();
    }
}
#endif
