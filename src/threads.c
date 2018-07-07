#include "common.h"
#include "fdbm.h"
#include "threads.h"
#include "pipe.h"
#include "pipe_util.h"

// this is usefull!
struct pipe_bridge_t {
	pipe_t* gate;
	pipe_consumer_t* from;
	pipe_producer_t* to;
};
typedef struct pipe_bridge_t pipe_bridge_t;

#define BUFFER_CHUNKS 20

int wasfreeCORE = 0;

INVISIBLE void attach_to_core(pthread_attr_t* attr, int i) {
	cpu_set_t core_i;
	CPU_ZERO(&core_i);
	CPU_SET(i, &core_i);
	pthread_attr_setaffinity_np(attr, sizeof(cpu_set_t), &core_i);
}

INVISIBLE int get_freeCORE(int thisCORE, int manyCORES) {
	for (register int i = wasfreeCORE; i < manyCORES; ++i) {
		if (i != thisCORE) {
			wasfreeCORE = i;
			debug("Free core %d found!", wasfreeCORE);
			return wasfreeCORE++;
		}
	}
	warning("No core is free! Falling back to the process core %d", thisCORE);
	return thisCORE;
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
	int thisCORE = sched_getcpu();
	int manyCORES = sysconf(_SC_NPROCESSORS_ONLN);
	debug("main core %d (out of %d)", thisCORE, manyCORES);

	// A tribute to pipes and pipelines...
	pipe_t* pipe_into_fdbm = pipe_new(get_frame_bytes(), SAMPLES_COUNT * BUFFER_CHUNKS);
	pipe_t* pipe_from_fdbm = pipe_new(get_frame_bytes(), SAMPLES_COUNT * BUFFER_CHUNKS);

	pipe_producer_t* pipe_audio_in = pipe_producer_new(pipe_into_fdbm);
	pipe_consumer_t* pipe_fdbm_in  = pipe_consumer_new(pipe_into_fdbm);
	pipe_free(pipe_into_fdbm);

	pipe_producer_t* pipe_fdbm_out  = pipe_producer_new(pipe_from_fdbm);
	pipe_consumer_t* pipe_audio_out = pipe_consumer_new(pipe_from_fdbm);

	pipe_bridge_t fdbm_bridge = { .gate = pipe_from_fdbm, .from = pipe_fdbm_in, .to = pipe_fdbm_out };

	playback_init();
	attach_to_core(&attr_playback, get_freeCORE(thisCORE, manyCORES));
	if (pthread_create(&audio_playback_process, &attr_playback, thread_playback_audio, pipe_audio_out)) {
		error("audio_playback_process init failed"); perror(NULL);
	}

	capture_init();
	attach_to_core(&attr_capture, get_freeCORE(thisCORE, manyCORES));
	if (pthread_create(&audio_capture_process, &attr_capture, thread_capture_audio, pipe_audio_in)) {
		error("audio_capture_process init failed"); perror(NULL);
	}

	attach_to_core(&attr_fdbm, get_freeCORE(thisCORE, manyCORES));
	if(pthread_create(&fdbm_process, &attr_fdbm, thread_fdbm_fork, &fdbm_bridge)) {
		error("fdbm_process init failed"); perror(NULL);
	}

	pthread_join(fdbm_process, NULL);

	pthread_join(audio_capture_process, NULL);
	capture_end();

	pthread_join(audio_playback_process, NULL);
	playback_end();

	pipe_free(pipe_from_fdbm);

	pipe_producer_free(pipe_audio_in);
	pipe_consumer_free(pipe_fdbm_in);
	pipe_producer_free(pipe_fdbm_out);
	pipe_consumer_free(pipe_audio_out);

}

void* thread_capture_audio(void* parameters) {

	debug("thread_capture_audio: init...");
	// log_printf("CAPTURE Process is running...\n");

	pipe_producer_t* capture = (pipe_producer_t*)parameters;

	// Well! this what malloc calls its return variable..
	char* victime = malloc(RAW_BUFFER_SIZE);
	debug("audio buffer allocated");

	long chunk_capture_count = 0;

	// setscheduler();

    int r, ok = 1;

	debug("thread_capture_audio: running...");
    while (ok) {
		if ((r = capture_read(victime, RAW_BUFFER_SIZE)) < 0) ok = 0;
		pipe_push(capture, victime, SAMPLES_COUNT);
		debug("chunk %lu pushed to the pipe!", chunk_capture_count++);
    }

	free(victime);
    debug("audio buffer freed");

	pthread_exit(NULL);
}

// void* thread_capture_audio(void* parameters) {
//
// 	debug("thread_capture_audio: init...");
// 	// log_printf("CAPTURE Process is running...\n");
//
// 	audio_buffer_t capture = (audio_buffer_t*)parameters;
// 	char* victime = malloc(BUFFER_CHUNKS * RAW_BUFFER_SIZE * get_frame_bytes());
//
// 	pthread_mutex_lock(&mutex_audio_buffer);
// 	capture->buffer = victime;
// 	capture->len = BUFFER_CHUNKS * RAW_BUFFER_SIZE;
//
// 	capture->write_to = capture->buffer;
// 	capture->written = 0;
//
// 	capture->read_from = NULL;
// 	capture->read = 0;
// 	pthread_mutex_unlock(&mutex_audio_buffer);
// 	debug("audio buffer allocated");
//
// 	char *current_buffer_chunk = NULL;
// 	int last_chunk_captured = 0;
// 	long chunk_capture_count = 0;
//
// 	current_buffer_chunk = audio_buffer;
//
// 	// setscheduler();
//
//     int r, ok = 1;
//
// 	debug("thread_capture_audio: running...");
//     while (ok) {
// 		if ((r = capture_read(capture->write_to, RAW_BUFFER_SIZE)) < 0) ok = 0;
// 		if (pthread_mutex_trylock(&mutex_audio_buffer) != EBUSY) {
// 			capture->read_from = capture->write_to;
// 			capture->written += r;
// 			if (capture->written == capture->len) {
// 				capture->written = 0;
// 				if (!capture->read) {
// 					// overrun error
// 					error("overrun!!!");
// 				}
// 			}
// 			capture->write_to = capture->buffer + capture->written;
// 			pthread_mutex_unlock(&mutex_audio_buffer);
// 			++chunk_capture_count;
// 			debug("chunk %lu captured", chunk_capture_count);
// 			sleep_ms(4);
//         } else {
// 			debug("mutex not acquiered! waiting for 10 ms");
// 			sleep_ms(10);
// 		}
//     }
//
// 	free(capture->buffer);
//     debug("audio buffer freed");
//
// 	pthread_exit(NULL);
// }

void* thread_playback_audio(void* parameters) {

	debug("thread_playback_audio: init...");
	// log_printf("PLAYBACK Process is running...\n");

	pipe_consumer_t* play = (pipe_consumer_t*)parameters;

	char* victime = malloc(RAW_BUFFER_SIZE);
	debug("audio buffer allocated");

	long chunk_play_count = 0;

	// setscheduler();

    int ok = 1;

	debug("thread_playback_audio: running...");
    while (ok) {
		while(pipe_pop(play, victime, SAMPLES_COUNT)) {
			if (playback_write(victime, RAW_BUFFER_SIZE) < 0) {
				ok = 0; break;
			}
			debug("chunk %lu played from pipe\n", ++chunk_play_count);
		}
    }

	free(victime);
	debug("audio buffer freed");

	pthread_exit(NULL);
}

// void* thread_playback_audio(void* parameters) {
//
// 	debug("thread_playback_audio: init...");
// 	// log_printf("PLAYBACK Process is running...\n");
//
// 	audio_buffer_t play = (audio_buffer_t*)parameters;
//
// 	char* victime = malloc(RAW_BUFFER_SIZE);
// 	long chunk_play_count = 0;
//
// 	// setscheduler();
//
//     int ok = 1;
//
// 	debug("thread_playback_audio: running...");
//     while (ok) {
//
// 		if (pthread_mutex_trylock(&mutex_audio_buffer) == EBUSY) {
// 			sleep_ms(10);
// 		} else {
// 			memcpy(victime, play->read_from, RAW_BUFFER_SIZE * get_frame_bytes());
// 			if () {
// 				startFDBM = 1;
// 				for (;;) {
// 					if (FDBMdone) {
// 						FDBMdone = 0;
// 						if (playback_write(victime, RAW_BUFFER_SIZE) < 0) ok = 0;
//
//
// 						++chunk_play_count;
// 						log_printf("chunk %lu played\n", chunk_play_count);
// 					} else {
// 						sleep_ms(5);
// 					}
// 				}
// 			} else {
// 				pthread_mutex_unlock(&mutex_audio_buffer);
// 			}
// 		}
//     }
//
// 	free(victime);
//
// 	pthread_exit(NULL);
// }

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
// 	audio_buffer = malloc(RAW_BUFFER_SIZE * get_frame_bytes());
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

INVISIBLE void fork_me(void* (*routine)(void*), void* parameters) {
	pthread_t lost_child;
	pthread_attr_t attr_lost_child;
	pthread_attr_init(&attr_lost_child);
	attach_to_core(&attr_lost_child, sched_getcpu());
	if(pthread_create(&lost_child, NULL, routine, parameters)) {
		error("fork failled!"); perror(NULL);
	}
	pthread_detach(lost_child);
}

struct lot_of_parameters {
	pipe_bridge_t bridge;
	char* buffer;
};

void* thread_fdbm_fork(void* parameters) {

	debug("thread_fdbm_fork: init...");
	pipe_bridge_t bridge = *(pipe_bridge_t*)parameters;

	struct lot_of_parameters *passed = malloc(sizeof(struct lot_of_parameters));
	passed->buffer = malloc(RAW_BUFFER_SIZE);
	debug("local buffer allocated");

	long chunk_fork_count = 0;

	debug("thread_fdbm_fork: running...");
	while (pipe_pop(bridge.from, passed->buffer, SAMPLES_COUNT)) {
		debug("chunk %lu in fork\n", ++chunk_fork_count);
		// passed->bridge = bridge;
		// thread_fdbm(passed);
		// fork_me(thread_fdbm, passed);
		sleep_ms(10);
	}

	free(passed->buffer);
	free(passed);
	pthread_exit(NULL);
}

#define secured_stuff(mutex, stuff) do { \
    pthread_mutex_lock(&mutex);          \
    do { stuff; } while(0);      		 \
    pthread_mutex_unlock(&mutex);        \
 } while(0)

int global_fdbm_running = 0;
pthread_mutex_t mutex_fdbm = PTHREAD_MUTEX_INITIALIZER;

void* thread_fdbm(void* parameters) {

	int local_fdbm_running;
	secured_stuff(mutex_fdbm, local_fdbm_running = ++global_fdbm_running);

	debug("thread_fdbm(%d): init...", local_fdbm_running);

	struct lot_of_parameters catched = *(struct lot_of_parameters*)parameters;

	char buffer[RAW_BUFFER_SIZE];
	memcpy(buffer, catched.buffer, RAW_BUFFER_SIZE);
	debug("local buffer allocated");

	debug("thread_fdbm(%d): running...", local_fdbm_running);
	// applyFDBM_simple1(buffer, SAMPLES_COUNT, 0);

	pipe_push(catched.bridge.to, buffer, SAMPLES_COUNT);
	debug("thread_fdbm(%d): running...", local_fdbm_running);

	pthread_exit(NULL);
}
