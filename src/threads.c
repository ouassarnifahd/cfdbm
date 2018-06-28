#include "common.h"
#include "alsa_wrapper.h"
#include "buffer_data.h"
#include "fdbm.h"
#include "threads.h"

void threads_init() {
	debug("Threads_init:");

	pthread_t audio_process, fdbm_process;
	if (pthread_create(&audio_process, NULL, thread_audio, NULL)) {
		error("audio_process init failed"); perror(NULL);
	}

	// thread_audio(NULL);

	// if(pthread_create(&fdbm_process, NULL, thread_fdbm, NULL)) {
	// 	error("fdbm_process init failed"); perror(NULL);
	// }


	pthread_join(audio_process, NULL);
	// pthread_join(fdbm_process, NULL);

}

void* thread_audio(void* parameters) {

	debug("thread_audio_capture: running...");
	log_printf("AUDIO is running...\n");

	char *audio_buffer;

	capture_init();
	playback_init();

	audio_buffer = malloc(BUFFER_SIZE * frame_bytes);
	debug("audio buffer allocated");

	setscheduler();

    int r, ok = 1;

    while (ok) {
        // tsc1 = get_cyclecount();

        if ((r = capture_read(audio_buffer, BUFFER_SIZE)) < 0)
            ok = 0;
        else {
            applyFBDM_simple1(audio_buffer, r, 0);

            if (playback_write(audio_buffer, r) < 0)
                ok = 0;
        }
        // tsc2 = get_cyclecount();
        // warning("loop cycle time %lu", get_cyclediff(tsc1, tsc2));
    }

	capture_end();
	playback_end();

	free(audio_buffer);
    debug("audio buffer freed");

	pthread_exit(NULL);
}

void* thread_opencv(void* parameters);

void* thread_AVstream(void* parameters);

void* thread_fdbm(void* parameters) {

	//applyFBDM_simple1();

}
