#include "common.h"
#include "fdbm.h"
#include "threads.h"
#include "pipe.h"
// #include "pipe_util.h"
// #include "mpscq.h"

// this is usefull!
struct pipe_bridge_t {
	pipe_consumer_t* from;
	pipe_producer_t* to;
};
typedef struct pipe_bridge_t pipe_bridge_t;

#define BUFFER_CHUNKS 50

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

	pipe_producer_t* pipe_fdbm_out  = pipe_producer_new(pipe_from_fdbm);
	pipe_consumer_t* pipe_audio_out = pipe_consumer_new(pipe_from_fdbm);

	pipe_bridge_t* fdbm_bridge = malloc(sizeof(pipe_bridge_t));
	fdbm_bridge->from = pipe_fdbm_in;
	fdbm_bridge->to = pipe_fdbm_out;

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
	if(pthread_create(&fdbm_process, &attr_fdbm, thread_fdbm_fork, fdbm_bridge)) {
		error("fdbm_process init failed"); perror(NULL);
	}

	pthread_join(fdbm_process, NULL);

	pthread_join(audio_capture_process, NULL);
	capture_end();

	pthread_join(audio_playback_process, NULL);
	playback_end();

	free(fdbm_bridge);

	pipe_producer_free(pipe_audio_in);
	pipe_consumer_free(pipe_fdbm_in);
	pipe_producer_free(pipe_fdbm_out);
	pipe_consumer_free(pipe_audio_out);

	pipe_free(pipe_from_fdbm);
	pipe_free(pipe_into_fdbm);
}

void* thread_capture_audio(void* parameters) {

	debug("thread_capture_audio: init...");
	// log_printf("CAPTURE Process is running...\n");

	pipe_producer_t* capture = (pipe_producer_t*)parameters;

	// Well! this what malloc calls its return variable..
	char* victime = malloc(RAW_BUFFER_SIZE);
	debug("audio buffer allocated");

	long chunk_capture_count = 0;

	// set RR realtime prio
	setscheduler(1);

    int r, ok = 1;

	debug("thread_capture_audio: running...");
    while (ok) {
		if ((r = capture_read(victime, RAW_BUFFER_SIZE)) < 0) ok = 0;
		pipe_push(capture, victime, SAMPLES_COUNT);
		sleep_ms(10);
		debug("chunk %lu pushed to the pipe!", chunk_capture_count++);
    }

	free(victime);
    debug("audio buffer freed");

	pthread_exit(NULL);
}

void* thread_playback_audio(void* parameters) {

	debug("thread_playback_audio: init...");
	// log_printf("PLAYBACK Process is running...\n");

	pipe_consumer_t* play = (pipe_consumer_t*)parameters;

	char* victime = malloc(RAW_BUFFER_SIZE);
	debug("audio buffer allocated");

	long chunk_play_count = 0;

	// set RR realtime prio
	setscheduler(1);

    int ok = 1;

	debug("thread_playback_audio: running...");
    while (ok) {
		while(pipe_pop(play, victime, SAMPLES_COUNT)) {
			if (playback_write(victime, RAW_BUFFER_SIZE) < 0) {
				ok = 0; break;
			}
			debug("chunk %lu played from pipe\n", ++chunk_play_count);
			sleep_ms(10);
			// error("ENDING RUN TEST");
		}
    }

	free(victime);
	debug("audio buffer freed");

	pthread_exit(NULL);
}

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
	pipe_bridge_t* bridge = (pipe_bridge_t*)parameters;

	struct lot_of_parameters *passed = malloc(sizeof(struct lot_of_parameters));
	memcpy(&passed->bridge, bridge, sizeof(pipe_bridge_t));
	passed->buffer = malloc(RAW_BUFFER_SIZE);
	debug("local buffer allocated");

	// set RR realtime prio
	setscheduler(2);

	debug("thread_fdbm_fork: running...");
	while (pipe_pop(passed->bridge.from, passed->buffer, SAMPLES_COUNT)) {
		fork_me(thread_fdbm, passed);
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

	// set RR realtime prio
	setscheduler(4);

	debug("thread_fdbm(%d): running...", local_fdbm_running);
	applyFDBM_simple1(buffer, SAMPLES_COUNT, DOA_CENTER);

	pipe_push(catched.bridge.to, buffer, SAMPLES_COUNT);
	debug("thread_fdbm(%d): Done!", local_fdbm_running);
	pthread_exit(NULL);
}
