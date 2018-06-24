#include "common.h"
#include "alsa_wrapper.h"
#include "fdbm.h"
#include "threads.h"

// pthread_t* main_process;
pthread_mutex_t mutex_audio_capture_buffer_RW;

pthread_t fdbm_process;

pthread_t audio_playback_process;
pthread_mutex_t mutex_audio_playback_buffer_RW;

pthread_t video_capture_process;
pthread_t AVstream_process;

void threads_init() {
	debug("Threads_init:");
	pthread_t audio_capture_process;

	if (pthread_create(&audio_capture_process, NULL, thread_audio_capture, NULL)) {
		error();
		perror(NULL);
	}

	// pthread_mutex_init(&mutex_audio_capture_buffer_RW, NULL);
	//
	// pthread_create(&fdbm_process, NULL, thread_fdbm, NULL);
	//
	// pthread_create(&audio_playback_process, NULL, thread_audio_playback, NULL);
	// pthread_mutex_init(&mutex_audio_playback_buffer_RW, NULL);


	pthread_join(&audio_capture_process, NULL);
	// pthread_join(&fdbm_process);
	// pthread_join(&audio_playback_process);

}

void* thread_audio_capture(void* parameters) {
	debug("thread_audio_capture: running...");

	snd_pcm_t *phandle, *chandle;

    // debug
    unsigned long tsc1, tsc2;

    char *buffer;
    int latency, UserInterruption = 0;
    int ok;
    ssize_t r;
    size_t frames_in = 0, frames_out = 0, in_max;

	buf_init(phandle, chandle, buffer, &latency);

    ok = 1;
    in_max = 0;
    while (ok) {
        tsc1 = get_cyclecount();

        if ((r = readbuf(chandle, buffer, latency, &frames_in, &in_max)) < 0)
            ok = 0;
        else {
            applyFBDM_simple1(buffer, r, 0);
            if (writebuf(phandle, buffer, r, &frames_out) < 0)
                ok = 0;
            /* use poll to wait for next event */
            snd_pcm_wait(chandle, 1000);
        }
        tsc2 = get_cyclecount();
        debug("loop cycle time %lu\n", get_cyclediff(tsc1, tsc2));
    }

	buf_end(phandle, chandle);

	pthread_exit(NULL);
}

void* thread_audio_playback(void* parameters);

void* thread_video_capture(void* parameters);

void* thread_AVstream(void* parameters);

void* thread_fdbm(void* parameters) {

	// applyFBDM_simple1();

}
