#include "common.h"
#include "alsa_wrapper.h"
#include "buffer_data.h"
#include "fdbm.h"
#include "threads.h"

#define BUFFER_CHUNKS 10

struct audio_buffer_struct {
	char* buffer;
	size_t len;
	int chunk_w;
	int chunk_r;
};
typedef struct audio_buffer_struct audio_buffer_t;

audio_buffer_t ITD_audio_buffer = {NULL, 0, 0, 0};

#define NANO_SECOND_MULTIPLIER 1000000L
#define sleep_ms(ms) nanosleep((const struct timespec[]){{0, (ms * NANO_SECOND_MULTIPLIER)}}, NULL)

pthread_mutex_t mutex_audio_buffer = PTHREAD_MUTEX_INITIALIZER;

int startFDBM = 0, FDBMdone = 0;

static inline __attribute__((always_inline)) void attach_to_core(pthread_attr_t* attr, int i) {
	cpu_set_t core_i;
	CPU_ZERO(&core_i);
	CPU_SET(i, &core_i);
	pthread_attr_setaffinity_np(attr, sizeof(cpu_set_t), &core_i);
}

void threads_init() {
	debug("Threads_init:");

	pthread_t audio_capture_process, audio_playback_process, fdbm_process;

	pthread_attr_t attr_capture, attr_playback, attr_fdbm;
	pthread_attr_init(&attr_capture);
	pthread_attr_init(&attr_playback);
	pthread_attr_init(&attr_fdbm);

	// printf("ID: %lu, CPU: %d\n", pthread_self(), sched_getcpu());
	// int N_cores = sysconf(_SC_NPROCESSORS_ONLN);

	playback_init();
	attach_to_core(&attr_playback, 1);
	if (pthread_create(&audio_playback_process, &attr_playback, thread_playback_audio, NULL)) {
		error("audio_playback_process init failed"); perror(NULL);
	}

	capture_init();
	attach_to_core(&attr_capture, 0);
	if (pthread_create(&audio_capture_process, &attr_capture, thread_capture_audio, NULL)) {
		error("audio_capture_process init failed"); perror(NULL);
	}

	attach_to_core(&attr_fdbm, 2);
	if(pthread_create(&fdbm_process, &attr_fdbm, thread_fdbm, NULL)) {
		error("fdbm_process init failed"); perror(NULL);
	}

	pthread_join(fdbm_process, NULL);

	pthread_join(audio_capture_process, NULL);
	capture_end();

	pthread_join(audio_playback_process, NULL);
	playback_end();

}

void* thread_capture_audio(void* parameters) {

	debug("thread_capture_audio: init...");
	// log_printf("CAPTURE Process is running...\n");

	char *audio_buffer = NULL;
	char *current_buffer_chunk = NULL;
	int last_chunk_captured = 0;
	long chunk_capture_count = 0;

	audio_buffer = malloc(BUFFER_CHUNKS * RAW_BUFFER_SIZE * frame_bytes);
	current_buffer_chunk = audio_buffer;
	debug("audio buffer allocated");

	// setscheduler();

    int r, ok = 1;

	debug("thread_capture_audio: running...");
    while (ok) {
        long tsc = get_cyclecount();
		if ((r = capture_read(current_buffer_chunk, RAW_BUFFER_SIZE)) < 0)
			ok = 0;
		if (pthread_mutex_trylock(&mutex_audio_buffer) != EBUSY) {
			ITD_audio_buffer.buffer = current_buffer_chunk;
			ITD_audio_buffer.len = RAW_BUFFER_SIZE;
			ITD_audio_buffer.chunk_w = last_chunk_captured;
			pthread_mutex_unlock(&mutex_audio_buffer);
			debug("chunk %lu captured", chunk_capture_count);
			current_buffer_chunk = audio_buffer + last_chunk_captured * RAW_BUFFER_SIZE;
			last_chunk_captured = (last_chunk_captured + 1) % BUFFER_CHUNKS;
			++chunk_capture_count;
        }
        warning("loop cycle time %lf ms", get_timediff_ms(tsc, get_cyclecount()));
    }

	free(audio_buffer);
    debug("audio buffer freed");

	pthread_exit(NULL);
}

void* thread_playback_audio(void* parameters) {

	debug("thread_playback_audio: init...");
	// log_printf("PLAYBACK Process is running...\n");

	audio_buffer_t play = {NULL, 0, 0, 0};
	long chunk_play_count = 0;

	// setscheduler();

    int ok = 1;

	debug("thread_playback_audio: running...");
    while (ok) {

		if (pthread_mutex_trylock(&mutex_audio_buffer) == EBUSY) {
			sleep_ms(5);
		} else {
			play.buffer = ITD_audio_buffer.buffer;
			play.len = ITD_audio_buffer.len;
			ITD_audio_buffer.chunk_r = play.chunk_w;
			play.chunk_r = ITD_audio_buffer.chunk_w;
			if (play.buffer || play.chunk_w != play.chunk_r) {
				startFDBM = 1;
				for (;;) {
					if (FDBMdone) {
						FDBMdone = 0;
						if (playback_write(play.buffer, play.len) < 0) ok = 0;
						log_printf("chunk %lu played\n", chunk_play_count);
						play.chunk_w = (play.chunk_w + 1) % BUFFER_CHUNKS;
						++chunk_play_count;
					} else {
						sleep_ms(1);
					}
				}
			} else {
				pthread_mutex_unlock(&mutex_audio_buffer);
			}
		}
    }

	pthread_exit(NULL);
}

// void* thread_audio(void* parameters) {
//
// 	debug("thread_audio_capture: running...");
// 	log_printf("AUDIO is running...\n");
//
// 	char *audio_buffer;
//
// 	capture_init();
// 	playback_init();
//
// 	audio_buffer = malloc(RAW_BUFFER_SIZE * frame_bytes);
// 	debug("audio buffer allocated");
//
// 	setscheduler();
//
//     int r, ok = 1;
//
//     while (ok) {
//         long tsc = get_cyclecount();
//
//         if ((r = capture_read(audio_buffer, RAW_BUFFER_SIZE)) < 0)
//             ok = 0;
//         else {
// 			long tsc1 = get_cyclecount();
// 			applyFBDM_simple1(audio_buffer, r, 0);
// 			long tsc2 = get_cyclecount();
// 			warning("fdbm cycle time %lu", get_cyclediff(tsc1, tsc2));
//             if (playback_write(audio_buffer, r) < 0)
//                 ok = 0;
//         }
//         warning("loop cycle time %lu", get_cyclediff(tsc, get_cyclecount()));
//     }
//
// 	capture_end();
// 	playback_end();
//
// 	free(audio_buffer);
//     debug("audio buffer freed");
//
// 	pthread_exit(NULL);
// }

void* thread_fdbm(void* parameters) {

	debug("thread_fdbm: init...");
	// log_printf("FDBM Process is running...\n");

	audio_buffer_t chunk = {NULL, 0, 0, 0};

	debug("thread_fdbm: running...");
	for (;;) {
		if (startFDBM) {
			startFDBM = 0;
			if (ITD_audio_buffer.buffer) {
				chunk.buffer = ITD_audio_buffer.buffer;
				chunk.len = ITD_audio_buffer.len;
				pthread_mutex_unlock(&mutex_audio_buffer);
				applyFBDM_simple1(chunk.buffer, chunk.len, 0);
				FDBMdone = 1;
			} else {
				pthread_mutex_unlock(&mutex_audio_buffer);
			}
		} else {
			sleep_ms(5);
		}
	}

	pthread_exit(NULL);
}
