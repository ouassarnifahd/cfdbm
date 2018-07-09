// #undef __DEBUG__

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

// time aware buffer
struct tsc_buffer {
	char* x;
	struct timespec rt_start;
	struct timespec rt_end;
	double time_start;
	double time_len;
	double time_end;
};
typedef struct tsc_buffer timed_buffer_t;

typedef void* (*routine_t) (void*);

#define BUFFER_CHUNKS 50

INVISIBLE void attach_to_core(pthread_attr_t* attr, int i) {
	cpu_set_t core_i;
	CPU_ZERO(&core_i);
	CPU_SET(i, &core_i);
	pthread_attr_setaffinity_np(attr, sizeof(cpu_set_t), &core_i);
}

// initial search index
int wasfreeCORE = 0;

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

	pthread_t audio_capture_process, audio_playback_process, fdbm_process, openCV_process;

	pthread_attr_t attr_capture, attr_playback, attr_fdbm, attr_openCV;
	pthread_attr_init(&attr_capture);
	pthread_attr_init(&attr_playback);
	pthread_attr_init(&attr_fdbm);

	int thisCORE = sched_getcpu();
	int manyCORES = sysconf(_SC_NPROCESSORS_ONLN);
	debug("main core %d (out of %d)", thisCORE, manyCORES);

	log_printf("Audio pipe init...");
	// A tribute to pipes and pipelines...
	pipe_t* pipe_into_fdbm = pipe_new(get_frame_bytes(), SAMPLES_COUNT * BUFFER_CHUNKS);
	pipe_t* pipe_from_fdbm = pipe_new(get_frame_bytes(), SAMPLES_COUNT * BUFFER_CHUNKS);

	// First STEP
	pipe_producer_t* pipe_audio_in = pipe_producer_new(pipe_into_fdbm);
	pipe_consumer_t* pipe_fdbm_in  = pipe_consumer_new(pipe_into_fdbm);

	// Second STEP
	pipe_producer_t* pipe_fdbm_out  = pipe_producer_new(pipe_from_fdbm);
	pipe_consumer_t* pipe_audio_out = pipe_consumer_new(pipe_from_fdbm);

	// THE Bridge
	pipe_bridge_t* fdbm_bridge = malloc(sizeof(pipe_bridge_t));
	fdbm_bridge->from = pipe_fdbm_in;
	fdbm_bridge->to = pipe_fdbm_out;
	log_printf(" [ OK ]\n");

	// ALSA init...
	log_printf("ALSA starting...");
	playback_init();
	capture_init();
	log_printf(" [ OK ]\n");

	// Here the fun starts... Good luck!
	int audioIO_CORE = get_freeCORE(thisCORE, manyCORES);
	attach_to_core(&attr_capture, audioIO_CORE);
	if (pthread_create(&audio_capture_process, &attr_capture, thread_capture_audio, pipe_audio_in)) {
		error("audio_capture_process init failed"); perror(NULL);
	}

	int audioProcessingCORE = get_freeCORE(thisCORE, manyCORES);
	attach_to_core(&attr_fdbm, audioProcessingCORE);
	if(pthread_create(&fdbm_process, &attr_fdbm, thread_fdbm_fork, fdbm_bridge)) {
		error("fdbm_process init failed"); perror(NULL);
	}


	// delaaaaaaay
	sleep_ms(300);
	attach_to_core(&attr_playback, audioIO_CORE);
	if (pthread_create(&audio_playback_process, &attr_playback, thread_playback_audio, pipe_audio_out)) {
		error("audio_playback_process init failed"); perror(NULL);
	}

	// OpenCV init...

	// SIGHANDLING interface

	// The main core stops here and waits for all the others!!
	pthread_join(fdbm_process, NULL);
	pthread_join(audio_capture_process, NULL);
	pthread_join(audio_playback_process, NULL);

	// Return what you took! dont carry that weight!
	pthread_attr_destroy(&attr_capture);
	pthread_attr_destroy(&attr_playback);
	pthread_attr_destroy(&attr_fdbm);

	// ALSA end.
	capture_end();
	playback_end();
	log_printf("ALSA stopped");

	free(fdbm_bridge);

	pipe_producer_free(pipe_audio_in);
	pipe_consumer_free(pipe_fdbm_in);
	pipe_producer_free(pipe_fdbm_out);
	pipe_consumer_free(pipe_audio_out);

	pipe_free(pipe_from_fdbm);
	pipe_free(pipe_into_fdbm);
}

INVISIBLE void fork_me(routine_t routine, void* parameters) {
	// lost child
	pthread_t lost_child;
	pthread_attr_t attr_lost_child;
	pthread_attr_init(&attr_lost_child);
	pthread_attr_setdetachstate(&attr_lost_child, PTHREAD_CREATE_DETACHED);
	attach_to_core(&attr_lost_child, sched_getcpu());
	if(pthread_create(&lost_child, &attr_lost_child, routine, parameters)) {
		error("fork failled!"); perror(NULL);
	}
}

void* thread_capture_audio(void* parameters) {

	debug("thread_capture_audio: init...");
	// log_printf("CAPTURE Process is running...\n");

	pipe_producer_t* capture = (pipe_producer_t*)parameters;

	timed_buffer_t capt_buf;

	// char* buf = malloc(RAW_ALSA_BUFFER_SIZE);
	char buf[RAW_ALSA_BUFFER_SIZE];
	capt_buf.x = buf;

	long chunk_capture_count = 0;

	// set RR realtime prio
	setscheduler(10);

    int r, ok = 1;

	debug("thread_capture_audio: running...");
    while (ok) {
		capt_buf.rt_start = get_realtimecount();
		capt_buf.time_start = get_realtime_from_start();
		if ((r = capture_read(capt_buf.x, RAW_ALSA_BUFFER_SIZE)) < 0) ok = 0;
		capt_buf.rt_end = get_realtimecount();
		capt_buf.time_end = get_realtime_from_start();
		capt_buf.time_len = get_realtime_from(&capt_buf.rt_start);
		pipe_push(capture, capt_buf.x, RAW_TO_SAMPLES(RAW_ALSA_BUFFER_SIZE));
		++chunk_capture_count;
		debug("chunk[%lu] { from %lfs to %lfs in %lfs }\n",
			chunk_capture_count, capt_buf.time_start, capt_buf.time_end, capt_buf.time_len);
		// if(chunk_capture_count == 100) error("ENDING RUN TEST");
    }

	pthread_exit(NULL);
}

void* thread_playback_audio(void* parameters) {

	debug("thread_playback_audio: init...");
	// log_printf("PLAYBACK Process is running...\n");

	pipe_consumer_t* play = (pipe_consumer_t*)parameters;
	char victime[RAW_ALSA_BUFFER_SIZE];

	long chunk_play_count = 0;

	// set RR realtime prio
	setscheduler(10);

    int ok = 1;

	debug("thread_playback_audio: running...");
    while (ok) {
		while(pipe_pop(play, victime, RAW_TO_SAMPLES(RAW_ALSA_BUFFER_SIZE))) {
			if (playback_write(victime, RAW_ALSA_BUFFER_SIZE) < 0) {
				ok = 0; break;
			}
			++chunk_play_count;
			debug("chunk %lu played from pipe\n", chunk_play_count);
			// if(chunk_play_count == 100) error("ENDING RUN TEST");
		}
    }
	pthread_exit(NULL);
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
	passed->buffer = malloc(RAW_FDBM_BUFFER_SIZE);
	debug("local buffer allocated");

	// set RR realtime prio
	setscheduler(15);

	debug("thread_fdbm_fork: running...");
	while (pipe_pop(passed->bridge.from, passed->buffer, RAW_TO_SAMPLES(RAW_FDBM_BUFFER_SIZE))) {
		fork_me(thread_fdbm, passed);
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
	char buffer[RAW_FDBM_BUFFER_SIZE];
	memcpy(buffer, catched.buffer, RAW_FDBM_BUFFER_SIZE);

	// set RR realtime prio
	setscheduler(20);

	debug("thread_fdbm(%d): running...", local_fdbm_running);
	applyFDBM_simple1(buffer, RAW_TO_SAMPLES(RAW_FDBM_BUFFER_SIZE), DOA_CENTER);

	pipe_push(catched.bridge.to, buffer, RAW_TO_SAMPLES(RAW_FDBM_BUFFER_SIZE));
	debug("thread_fdbm(%d): Done!\n", local_fdbm_running);
	pthread_exit(NULL);
}
