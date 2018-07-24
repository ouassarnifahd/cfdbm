// #undef __DEBUG__

#include "common.h"
#include "fdbm.h"
#include "threads.h"
#include "pipe.h"

// this is usefull!
struct pipe_bridge_t {
	#ifdef __THRD_PARTY_PIPES__
	  pipe_generic_t* from;
	  pipe_generic_t* to;
	#else
	  int from;
	  int to;
	#endif
};
typedef struct pipe_bridge_t pipe_bridge_t;

// time aware buffer
struct tsc_buffer {
	char* data;
	#ifdef __DEBUG__
	  struct timespec rt_start;
	  struct timespec rt_end;
	  double time_start;
	  double time_len;
	  double time_end;
	#endif
};
typedef struct tsc_buffer timed_buffer_t;

typedef void* (*routine_t) (void*);

#define BUFFER_CHUNKS 100
#define DOA_BUFFER 	  20

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
			// debug("Free core %d found!", wasfreeCORE);
			return wasfreeCORE++;
		}
	}
	warning("No core is free! Falling back to the process core %d", thisCORE);
	return thisCORE;
}

void threads_init() {
	debug("Threads_init:");

	// Threads
	pthread_t audio_capture_process, audio_playback_process, fdbm_process, openCV_process;

	// Threads attributes
	pthread_attr_t attr_capture, attr_playback, attr_fdbm, attr_openCV;
	pthread_attr_init(&attr_playback);
	pthread_attr_init(&attr_capture);
	pthread_attr_init(&attr_openCV);
	pthread_attr_init(&attr_fdbm);

	// Thread CORES
	int thisCORE = sched_getcpu();
	int manyCORES = sysconf(_SC_NPROCESSORS_ONLN);
	debug("main core %d (out of %d)", thisCORE, manyCORES);

	// A tribute to pipes and pipelines...
	log_printf("PIPES init...\t");
	// THE Bridge
	void* fdbm_bridge = malloc(3 * sizeof(pipe_generic_t*));

	#ifdef __USE_ALSA__
	  int frame_bytes = 4;//input->buffer.bytes_per_frame;
	#else // __USE_PULSEAUDIO__
	  int frame_bytes = 4;
	#endif

	#ifdef __THRD_PARTY_PIPES__
	  pipe_t* pipe_into_fdbm = pipe_new(1, frame_bytes * SAMPLES_COUNT * BUFFER_CHUNKS);
	  pipe_t* pipe_from_fdbm = pipe_new(1, frame_bytes * SAMPLES_COUNT * BUFFER_CHUNKS);
	  // pipe_t* pipe_fdbm_doa  = pipe_new(sizeof(int), DOA_BUFFER);

	  // First STEP
	  pipe_producer_t* pipe_audio_in = pipe_producer_new(pipe_into_fdbm);
	  pipe_consumer_t* pipe_fdbm_in  = pipe_consumer_new(pipe_into_fdbm);

	  // Second STEP
	  pipe_producer_t* pipe_fdbm_out  = pipe_producer_new(pipe_from_fdbm);
	  pipe_consumer_t* pipe_audio_out = pipe_consumer_new(pipe_from_fdbm);

	  // Third STEP
	  // pipe_producer_t* pipe_doa_in  = pipe_producer_new(pipe_fdbm_doa);
	  // pipe_consumer_t* pipe_doa_out = pipe_consumer_new(pipe_fdbm_doa);

	  // THE Bridge
	  pipe_bridge_t* bridge = fdbm_bridge;
	  pipe_generic_t* pipes = fdbm_bridge;
	  bridge->from = pipe_fdbm_in;
	  bridge->to   = pipe_fdbm_out;
	  // pipes[2]     = pipe_doa_out;
	#else // __LINUX_PIPES__
	  // linux ITC (man pipe) TODO!!! next time
	  // http://tldp.org/LDP/lpg/node11.html
	  int* pipe_into_fdbm = malloc(2 * sizeof(int)); pipe(pipe_into_fdbm);
	  int* pipe_from_fdbm = malloc(2 * sizeof(int)); pipe(pipe_from_fdbm);
	  // int* pipe_fdbm_doa  = malloc(2 * sizeof(int)); pipe(pipe_fdbm_doa);

	  // First STEP
	  int* pipe_audio_in = &pipe_into_fdbm[1];
	  int* pipe_fdbm_in  = &pipe_into_fdbm[0];

	  // Second STEP
	  int* pipe_fdbm_out  = &pipe_from_fdbm[1];
	  int* pipe_audio_out = &pipe_from_fdbm[0];

	  // Third STEP
	  int* pipe_doa_in  = &pipe_fdbm_doa[1];
	  int* pipe_doa_out = &pipe_fdbm_doa[0];

	  // THE Bridge
	  pipe_bridge_t* bridge = fdbm_bridge;
	  int* pipes = fdbm_bridge;
	  bridge->from = *pipe_fdbm_in;
	  bridge->to   = *pipe_fdbm_out;
	  // pipes[2]     = *pipe_fdbm_doa;
	#endif // __THRD_PARTY_PIPES__

	log_printf(" [ OK ]\n");

	// AUDIO init...
	log_printf("AUDIO starting...");
	playback_init();
	capture_init();
	log_printf(" [ OK ]\n");

	// Assigning CORES
	int audioIO_CORE = get_freeCORE(thisCORE, manyCORES);
	debug("audioIO_CORE = %d", audioIO_CORE);
	int audioProcessingCORE = get_freeCORE(thisCORE, manyCORES);
	debug("fdbmCORE     = %d", audioProcessingCORE);
	int opencvCORE = get_freeCORE(thisCORE, manyCORES);
	debug("opencvCORE   = %d", opencvCORE);

	// Here the fun starts... Good luck!
	log_printf("AUDIO Playback...");
	attach_to_core(&attr_playback, audioIO_CORE);
	if (pthread_create(&audio_playback_process, &attr_playback, thread_playback_audio, pipe_audio_out)) {
		error("audio_playback_process init failed"); perror(NULL);
	}
	pthread_setname_np(audio_playback_process, "CFDBM playback");
	log_printf(" [ ON ]\n");

	log_printf("AUDIO Capture...\t");
	attach_to_core(&attr_capture, audioIO_CORE);
	if (pthread_create(&audio_capture_process, &attr_capture, thread_capture_audio, pipe_audio_in)) {
		error("audio_capture_process init failed"); perror(NULL);
	}
	pthread_setname_np(audio_capture_process, "CFDBM capture");
	log_printf(" [ ON ]\n");

	log_printf("FDBM worker...\t");
	attach_to_core(&attr_fdbm, audioProcessingCORE);
	if(pthread_create(&fdbm_process, &attr_fdbm, thread_fdbm_fork, fdbm_bridge)) {
			error("fdbm_process init failed"); perror(NULL);
		}
	pthread_setname_np(fdbm_process, "CFDBM worker");
	log_printf(" [ ON ]\n");

	// OpenCV init...
	log_printf("OpenCV face detection...");
	attach_to_core(&attr_openCV, audioProcessingCORE);
	// if(pthread_create(&openCV_process, &attr_openCV, thread_openCV, NULL)) {
	// 		error("fdbm_process init failed"); perror(NULL);
	// 	}
	pthread_setname_np(fdbm_process, "CFDBM doa");
	log_printf(" [ ON ]\n");
	// SIGHANDLING interface

	// The main core stops here and waits for all the others!!
	pthread_join(fdbm_process, NULL);
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

	// free THE Bridge
	free(fdbm_bridge);

	// pipeline.
	#ifdef __THRD_PARTY_PIPES__
	pipe_producer_free(pipe_audio_in);
	pipe_consumer_free(pipe_fdbm_in);
	pipe_producer_free(pipe_fdbm_out);
	pipe_consumer_free(pipe_audio_out);
	pipe_free(pipe_from_fdbm);
	pipe_free(pipe_into_fdbm);
	#else // __LINUX_PIPES__
	free(pipe_into_fdbm);
	free(pipe_from_fdbm);
	#endif
}

void* thread_capture_audio(void* parameters) {

	debug("thread_capture_audio: init...");
	// log_printf("CAPTURE Process is running...\n");

	#ifdef __THRD_PARTY_PIPES__
	pipe_producer_t* capture = (pipe_producer_t*)parameters;
	#else // __LINUX_PIPES__
	int capture = *(int *)parameters;
	debug("capture = %d", capture);
	#endif

	timed_buffer_t capt_buf;

	char buf[RAW_AUDIO_BUFFER_SIZE];
	capt_buf.data = buf;

	long chunk_capture_count = 0;

	// set RR realtime prio
	setscheduler(10);

    int ok = 1;

	debug("thread_capture_audio: running...");
	while (ok) {
		#ifdef __DEBUG__
		  capt_buf.rt_start = get_realtimecount();
		  capt_buf.time_start = get_realtime_from_start();
		#endif
		if (capture_read(capt_buf.data, SAMPLES_COUNT) < 0) ok = 0;
		++chunk_capture_count;
		#ifdef __DEBUG__
		  capt_buf.rt_end = get_realtimecount();
		  capt_buf.time_end = get_realtime_from_start();
		  capt_buf.time_len = get_realtime_from(&capt_buf.rt_start);
		  debug("chunk[%lu] captured { from %lfs to %lfs in %lfs }\n", chunk_capture_count,
			capt_buf.time_start, capt_buf.time_end, capt_buf.time_len);
		  // if(chunk_capture_count == 1000) break;
		#endif
		#ifdef __THRD_PARTY_PIPES__
		pipe_push(capture, capt_buf.data, RAW_AUDIO_BUFFER_SIZE);
		#else // __LINUX_PIPES__
		write(capture, capt_buf.data, RAW_AUDIO_BUFFER_SIZE);
		#endif
		sleep_ms(20);
	}

	pthread_exit(NULL);
}

void* thread_playback_audio(void* parameters) {

	debug("thread_playback_audio: init...");
	// log_printf("PLAYBACK Process is running...\n");

	#ifdef __THRD_PARTY_PIPES__
	pipe_consumer_t* play = (pipe_consumer_t*)parameters;
	#else // __LINUX_PIPES__
	int play = *(int *)parameters;
	debug("play = %d", play);
	#endif

	timed_buffer_t play_buf;

	char victime[RAW_AUDIO_BUFFER_SIZE];
	play_buf.data = victime;

	long chunk_play_count = 0;

	// set RR realtime prio
	setscheduler(10);

    int ok = 1;

	debug("thread_playback_audio: running...");
    while (ok) {
		#ifdef __THRD_PARTY_PIPES__
		while(pipe_pop(play, play_buf.data, RAW_AUDIO_BUFFER_SIZE))
		#else // __LINUX_PIPES__
		while(read(play, play_buf.data, RAW_AUDIO_BUFFER_SIZE) > 0)
		#endif
		{
			#ifdef __DEBUG__
			  play_buf.rt_start = get_realtimecount();
			  play_buf.time_start = get_realtime_from_start();
			#endif
			if (playback_write(play_buf.data, SAMPLES_COUNT) < 0) {
				ok = 0; break;
			}
			++chunk_play_count;
			#ifdef __DEBUG__
			  play_buf.rt_end = get_realtimecount();
			  play_buf.time_end = get_realtime_from_start();
			  play_buf.time_len = get_realtime_from(&play_buf.rt_start);
			  debug("chunk[%lu] played  { from %lfs to %lfs in %lfs }\n", chunk_play_count,
				play_buf.time_start, play_buf.time_end, play_buf.time_len);
			  // if(chunk_play_count == 1000) { ok = 0; break; }
			#endif
		}
    }
	pthread_exit(NULL);
}

void* thread_openCV(void* parameters) {

}

// Here is the FDBM...
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

#define MAX_INSTANCES 5

int forked = 0;
int global_fdbm_count = 0;
m_init(mutex_fdbm_fork);

void* thread_fdbm_fork(void* parameters) {

	debug("thread_fdbm_fork: init...");
	pipe_bridge_t* bridge = (pipe_bridge_t*)parameters;

	// set RR realtime prio
	setscheduler(10);

	// synchronization
	int local_forked = 0;
	int need_fork = 1;

	debug("thread_fdbm_fork: running...");
	for(;;)
	{
		// // if(signal) break;
		// debug("need_fork = %s", need_fork?"yes":"no");
		// if (need_fork) fork_me(thread_fdbm, bridge);
		// else error("FN!");
		// do {
		// 	secured_stuff(mutex_fdbm_fork, local_forked = forked);
		// 	// sleep_ms(10);
		// } while(!local_forked);
		// secured_stuff(mutex_fdbm_fork, need_fork = (global_fdbm_count < THREADS));

		char* buffer = malloc(RAW_FDBM_BUFFER_SIZE);

		// fetching audio
		#ifdef __THRD_PARTY_PIPES__
		if(pipe_pop(bridge->from, buffer, RAW_FDBM_BUFFER_SIZE)) {
		#else // __LINUX_PIPES__
		while(read(bridge->from, buffer, RAW_FDBM_BUFFER_SIZE) < 0);
		#endif

		// if (local_fdbm_running > 200 && local_fdbm_running < 215)
		debug("buffer in  @%X", buffer);
		applyFDBM_simple1(buffer, SAMPLES_COUNT, DOA_CENTER);
		debug("buffer out @%X", buffer);
		// sleep_ms(30);

		#ifdef __THRD_PARTY_PIPES__
		pipe_push(bridge->to, buffer, RAW_FDBM_BUFFER_SIZE); }
		#else // __LINUX_PIPES__
		write(bridge->to, buffer, RAW_FDBM_BUFFER_SIZE);
		#endif

		free(buffer);
	}

	pthread_exit(NULL);
}

int global_fdbm_running = 0;
m_init(mutex_fdbm);

void* thread_fdbm(void* parameters) {
	// set RR realtime prio
	setscheduler(20);

	// local routine count
	int local_fdbm_running;
	secured_stuff(mutex_fdbm, local_fdbm_running = ++global_fdbm_running; forked = 1);

	// synchronization
	secured_stuff(mutex_fdbm_fork, ++global_fdbm_count);
	debug("thread_fdbm(%d): init...", local_fdbm_running);

	pipe_bridge_t bridge = *(pipe_bridge_t*)parameters;
	char* buffer = malloc(RAW_FDBM_BUFFER_SIZE);
	debug("thread_fdbm(%d): buffer allocated", local_fdbm_running);

	// fetching audio
	#ifdef __THRD_PARTY_PIPES__
	if(pipe_pop(bridge.from, buffer, RAW_FDBM_BUFFER_SIZE)) {
	#else // __LINUX_PIPES__
	while(read(bridge.from, buffer, RAW_FDBM_BUFFER_SIZE) < 0);
	#endif

	debug("thread_fdbm(%d): running...", local_fdbm_running);
	// if (local_fdbm_running > 200 && local_fdbm_running < 215)
	// applyFDBM_simple1(buffer, RAW_TO_SAMPLES(RAW_FDBM_BUFFER_SIZE), DOA_CENTER);
	// sleep(2);

	#ifdef __THRD_PARTY_PIPES__
	pipe_push(bridge.to, buffer, RAW_FDBM_BUFFER_SIZE); }
	#else // __LINUX_PIPES__
	write(bridge.to, buffer, RAW_FDBM_BUFFER_SIZE);
	#endif

	free(buffer);
	secured_stuff(mutex_fdbm_fork, --global_fdbm_count);
	debug("thread_fdbm(%d): Done!\n", local_fdbm_running);
	pthread_exit(NULL);
}
