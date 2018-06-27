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
	pthread_t audio_process;

	// if (pthread_create(&audio_process, NULL, thread_audio_capture, NULL)) {
	// 	error(NULL);
	// 	perror(NULL);
	// }

	thread_audio(NULL);

	// pthread_mutex_init(&mutex_audio_capture_buffer_RW, NULL);
	//
	// pthread_create(&fdbm_process, NULL, thread_fdbm, NULL);
	//
	// pthread_create(&audio_playback_process, NULL, thread_audio_playback, NULL);
	// pthread_mutex_init(&mutex_audio_playback_buffer_RW, NULL);


	// pthread_join(&audio_capture_process, NULL);
	// pthread_join(&fdbm_process);
	// pthread_join(&audio_playback_process);

}

void* thread_audio(void* parameters) {
	debug("thread_audio_capture: running...");

	char *capture_buffer, *playback_buffer;

    // debug
    unsigned long tsc1, tsc2;

	capture_end();
	capture_buffer = malloc(BUFFER_SIZE * frame_bytes);
	debug("capture buffer allocated");

	playback_init();
	playback_buffer = malloc(BUFFER_SIZE * frame_bytes);
    debug("playback buffer allocated");

    int r, ok = 1;

    while (ok) {
        tsc1 = get_cyclecount();

        if ((r = capture_read(capture_buffer, BUFFER_SIZE)) < 0)
            ok = 0;
        else {
            applyFBDM_simple1(buffer, r, 0);
            if (playback_write(buffer, r) < 0)
                ok = 0;
        }
        tsc2 = get_cyclecount();
        debug("loop cycle time %lu\n", get_cyclediff(tsc1, tsc2));
    }

	capture_end();
	free(capture_buffer);
    debug("capture buffer freed");

	playback_end();
	free(playback_buffer);
    debug("playback buffer freed");

	pthread_exit(NULL);
}

void* thread_video_capture(void* parameters);

void* thread_AVstream(void* parameters);

void* thread_fdbm(void* parameters) {

	// applyFBDM_simple1();

}
