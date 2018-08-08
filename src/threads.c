// #undef __DEBUG__
#include "common.h"
#include "fdbm.h"
#include "threads.h"
#include "pipe.h"
#include "thpool.h"
#include "wav.h"

// demo flags
#define DEMO 1

#if (DEMO == 1) && !defined(__DEBUG__)
  // WARNING this code is in use! Dont modify it unless you know what you are doing!
  #include <time.h>
  #define get_cyclecount() clock()
  #define get_cputimediff(tsc1, tsc2) (double)(tsc2-tsc1)/CLOCKS_PER_SEC

  INVISIBLE double TimeSpecToSeconds(struct timespec* ts) {
    return (double)ts->tv_sec + (double)ts->tv_nsec / 1000000000.0;
  }
  INVISIBLE struct timespec get_realtimecount() {
    struct timespec rtc;
    if(clock_gettime(CLOCK_MONOTONIC, &rtc));
    return rtc;
  }

  #define get_realtimediff(tsc1, tsc2) (TimeSpecToSeconds(tsc2)-TimeSpecToSeconds(tsc1))

  clock_t tsc_start;
  struct timespec rtc_start;

  #define init_timestamp() do { tsc_start = get_cyclecount(); } while(0)
  #define init_localtime() do { rtc_start = get_realtimecount(); } while(0)
  #define get_cputime_from_start() get_cputimediff(tsc_start, get_cyclecount())

  INVISIBLE double get_realtime_from(struct timespec* t_start) {
    struct timespec t_now = get_realtimecount();
    return get_realtimediff(t_start, &t_now);
  }

  INVISIBLE double get_realtime_from_start() {
    return get_realtime_from(&rtc_start);
  }
#endif

// temp config
char wave_file[20];
int manual_doa;
// #ifdef OFFLINE_FDBM

// this is usefull!
struct pipe_bridge_t {
	#ifdef __THRD_PARTY_PIPES__
	  pipe_consumer_t* from;
	  pipe_producer_t* to;
	#else
	  int from;
	  int to;
	#endif
};
typedef struct pipe_bridge_t pipe_bridge_t;

#define DOA_MAX 3

struct doa_t {
	int detected;
	int theta[DOA_MAX];
};
typedef struct doa_t doa_t;

// timing ...
struct period_t {
	struct timespec rt_start;
	double time_start;
	double time_len;
	double time_end;
};
typedef struct period_t period_t;

#define PERIOD_BEGIN(period) do { \
	(period)->rt_start = get_realtimecount(); \
	(period)->time_start = get_realtime_from_start(); \
} while(0)
#define PERIOD_FINISH(period) do { \
	(period)->time_end = get_realtime_from_start(); \
	(period)->time_len = get_realtime_from(&(period)->rt_start); \
} while(0)
#define PERIOD_START(period) (period)->time_start
#define PERIOD_END(period) (period)->time_end
#define PERIOD_LEN(period) (period)->time_len

#if (DEMO == 1)
period_t audio_latency = {0};
int audio_skiped = 0;
#define DEMO_TIME 5
#endif

// time aware buffer
struct tsc_buffer {
	char* data;
	period_t period;
};
typedef struct tsc_buffer timed_buffer_t;

struct overlap_buffer {
	int16_t prev[AUDIO_SAMPLES_COUNT];
	int16_t current[AUDIO_SAMPLES_COUNT];
};
typedef struct overlap_buffer overlap_buffer_t;

#define BUFFER_LATENCY 12

typedef void* (*routine_t) (void*);

#define AUDIO_CHUNKS 200
#define   DOA_CHUNKS 20

INVISIBLE void attach_to_core(pthread_attr_t* attr, int i) {
	cpu_set_t core_i;
	CPU_ZERO(&core_i);
	CPU_SET(i, &core_i);
	pthread_attr_setaffinity_np(attr, sizeof(cpu_set_t), &core_i);
}

INVISIBLE void attach_to_2cores(pthread_attr_t* attr, int i, int j) {
	cpu_set_t core_i, core_j;
	CPU_ZERO(&core_i); CPU_ZERO(&core_j);
	CPU_SET(i, &core_i); CPU_SET(j, &core_j);
	CPU_OR(&core_i, &core_i, &core_j);
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

INVISIBLE pthread_t thread_single_new(char* name, int core, routine_t routine, void* parameters) {
	pthread_t thread;
	pthread_attr_t attr; pthread_attr_init(&attr);
	attach_to_core(&attr, core);
	if (pthread_create(&thread, &attr, routine, parameters)) {
		warning("%s init failed", name); perror(NULL); exit(1);
	}
	pthread_setname_np(thread, name);
	return thread;
}

INVISIBLE pthread_t thread_dual_new(char* name, int core1, int core2, routine_t routine, void* parameters) {
	pthread_t thread;
	pthread_attr_t attr; pthread_attr_init(&attr);
	attach_to_2cores(&attr, core1, core2);
	if (pthread_create(&thread, &attr, routine, parameters)) {
		warning("%s init failed", name); perror(NULL); exit(1);
	}
	pthread_setname_np(thread, name);
	return thread;
}

struct container1 {
    pipe_bridge_t bridge;
    pipe_consumer_t* doa;
};

void threads_init() {
	debug("Threads_init:");

	#if (DEMO == 1) && !defined(__DEBUG__)
	init_localtime();
	#endif

	// SIGHANDLING interface

	// The Thread...
	pthread_t fdbm_process;

	// CORES init...
	log_printf("CORES init...	 ");
		int thisCORE  = sched_getcpu();
		int manyCORES = sysconf(_SC_NPROCESSORS_ONLN);
		debug("mainCORE     = %d (out of %d)", thisCORE, manyCORES);
		int audioIO_CORE = get_freeCORE(thisCORE, manyCORES);
		debug("audioIO_CORE = %d", audioIO_CORE);
		int audioProcessingCORE1 = get_freeCORE(thisCORE, manyCORES);
		int opencvCORE = get_freeCORE(thisCORE, manyCORES);
		debug("opencvCORE   = %d", opencvCORE);
		int audioProcessingCORE2 = get_freeCORE(thisCORE, manyCORES);
		debug("fdbmCORE     = %d,%d", audioProcessingCORE1, audioProcessingCORE2);
	log_printf(" [ OK ]\n");

	// AUDIO init...
	log_printf("AUDIO init...    ");
		playback_init();
		capture_init();
	log_printf(" [ OK ]\n");

	// AUDIO pipes...
	log_printf("AUDIO pipes...   ");
		#ifdef __USE_ALSA__
	  	  int sample_bytes = SAMPLE_BYTES; //input->buffer.bytes_per_frame;
		#else // __USE_PULSEAUDIO__ (not yet implemented)
	  	  int sample_bytes = 0;
		  #error "Please define the '__USE_ALSA__' Macro before including audio_wrapper.h"
		#endif

		#ifdef __THRD_PARTY_PIPES__
		  // THE Bridge
	  	  struct container1* fdbm_params = malloc(sizeof(struct container1));

		  // THE Pipes
	      pipe_t* pipe_into_fdbm = pipe_new(sample_bytes, FDBM_SAMPLES_COUNT * AUDIO_CHUNKS);
	      pipe_t* pipe_from_fdbm = pipe_new(sample_bytes, AUDIO_SAMPLES_COUNT * AUDIO_CHUNKS);
	      pipe_t* pipe_fdbm_doa  = pipe_new(sizeof(doa_t), DOA_CHUNKS);

	      // First STEP
	      pipe_producer_t* pipe_audio_in = pipe_producer_new(pipe_into_fdbm);
	      pipe_consumer_t* pipe_fdbm_in  = pipe_consumer_new(pipe_into_fdbm);

	      // Second STEP
	      pipe_producer_t* pipe_fdbm_out  = pipe_producer_new(pipe_from_fdbm);
	      pipe_consumer_t* pipe_audio_out = pipe_consumer_new(pipe_from_fdbm);

	      // Third STEP
	      pipe_producer_t* pipe_doa_in  = pipe_producer_new(pipe_fdbm_doa);
	      pipe_consumer_t* pipe_doa_out = pipe_consumer_new(pipe_fdbm_doa);

	      // THE Links
	      pipe_bridge_t* bridge = &fdbm_params->bridge;
	      bridge->from = pipe_fdbm_in;
	      bridge->to   = pipe_fdbm_out;
          fdbm_params->doa = pipe_doa_out;
	    #else // __LINUX_PIPES__ (not yet implemented)
	      // linux ITC (man pipe) TODO!!! next time
	      // http://tldp.org/LDP/lpg/node11.html
		  // THE Bridge
	  	  void* fdbm_bridge = malloc(sizeof(pipe_bridge_t) + sizeof(pipe_generic_t*));

		  // THE Pipes
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
	      // int* pipe_doa_in  = &pipe_fdbm_doa[1];
	      // int* pipe_doa_out = &pipe_fdbm_doa[0];

	      // THE Links
	      pipe_bridge_t* bridge = fdbm_bridge;
	      int** pipes = fdbm_bridge;
	      bridge->from = *pipe_fdbm_in;
	      bridge->to   = *pipe_fdbm_out;
	      // pipes[2]     = *pipe_fdbm_doa;
	    #endif // __THRD_PARTY_PIPES__
	log_printf(" [ OK ]\n");

	// AUDIO Playback...
	log_printf("AUDIO Playback...");
		thread_single_new("CFDBM playback", audioIO_CORE, thread_playback_audio, pipe_audio_out);
	log_printf(" [ OK ]\n");

	// AUDIO Capture...
	log_printf("AUDIO Capture... ");
		thread_single_new("CFDBM capture", audioIO_CORE, thread_capture_audio, pipe_audio_in);
	log_printf(" [ OK ]\n");

	// FDBM worker...
	log_printf("FDBM   worker... ");
		// #ifdef __arm__
		// fdbm_process = thread_dual_new("CFDBM pool", audioProcessingCORE1, audioProcessingCORE2, thread_fdbm_pool, fdbm_bridge);
		// #else // __i386__
		fdbm_process = thread_single_new("CFDBM pool", audioProcessingCORE1, thread_fdbm_pool, fdbm_params);
		// #endif
	log_printf(" [ OK ]\n");

    // temporary
	log_printf("FDBM   DOA = %d\n", manual_doa);

	// OpenCV worker...
	log_printf("OpenCV worker... ");
		thread_single_new("CFDBM camera", opencvCORE, thread_openCV, pipe_doa_in);
	log_printf(" [ OK ]\n");

	// The main thread stops here and waits for The thread !!
	pthread_join(fdbm_process, NULL);

	// PIPES destroy...
	log_printf("PIPES destroy... ");
		// free THE Bridge
		free(fdbm_params);

		// pipeline.
		#ifdef __THRD_PARTY_PIPES__
	  	  pipe_producer_free(pipe_audio_in);
	  	  pipe_consumer_free(pipe_fdbm_in);
	  	  pipe_producer_free(pipe_doa_in);
	  	  pipe_consumer_free(pipe_doa_out);
	  	  pipe_producer_free(pipe_fdbm_out);
	  	  pipe_consumer_free(pipe_audio_out);
	  	  pipe_free(pipe_from_fdbm);
	  	  pipe_free(pipe_into_fdbm);
	  	  pipe_free(pipe_fdbm_doa);
		#else // __LINUX_PIPES__
	  	  free(pipe_into_fdbm);
	      free(pipe_from_fdbm);
		#endif
	log_printf(" [ OK ]\n");

	// AUDIO close...
	log_printf("AUDIO close...   ");
		capture_end();
		playback_end();
	log_printf(" [ OK ]\n");
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

	#if (DEMO == 1)
	  FILE* wav_in = fopen("demo_input.wav","wb");
	  int sample_count = 0;
	  int first_buffer = 1;
	  write_wav_header(wav_in, CHANNELS, RATE, 2, DEMO_TIME * RATE);
	  wav_sample_t sample = {
        CHANNELS,
        2,
        2 * CHANNELS,
        NULL
      };
	#endif

	timed_buffer_t capt_buf;
	overlap_buffer_t capt_obuf;
	int16_t* samples = &capt_obuf;

	// char buf[RAW_AUDIO_BUFFER_SIZE];
	capt_buf.data = capt_obuf.current;

	long chunk_capture_count = 0;

	// set RR realtime prio
	setscheduler(20);

    int ok = 1;

	debug("thread_capture_audio: running...");
	while (ok) {
		#ifdef __DEBUG__
		  PERIOD_BEGIN(&capt_buf.period);
		#endif
		for (register int i = 0; i < AUDIO_SAMPLES_COUNT; ++i) {
			capt_obuf.prev[i] = capt_obuf.current[i];
		}
		if (capture_read(capt_obuf.current, SAMPLES_TO_FRAMES(AUDIO_SAMPLES_COUNT)) < 0) ok = 0;
		++chunk_capture_count;
		#ifdef __DEBUG__
		  PERIOD_FINISH(&capt_buf.period);
		  debug("%u = chunk[%lu] captured { from %lfs to %lfs in %lfs }\n",
		  	(unsigned int)AUDIO_SAMPLES_COUNT, chunk_capture_count,
			PERIOD_START(&capt_buf.period), PERIOD_END(&capt_buf.period),
			PERIOD_LEN(&capt_buf.period));
		  // if(chunk_capture_count == 1000) break;
		#endif
		#ifdef __THRD_PARTY_PIPES__
		pipe_push(capture, samples, FDBM_SAMPLES_COUNT);
		#else // __LINUX_PIPES__
		write(capture, capt_buf.data, SAMPLES_COUNT);
		#endif
		#if(DEMO == 1)
		  int16_t *sample_io = capt_buf.data;
		  int16_t sampleData[CHANNELS];
		  if (first_buffer) {
			  first_buffer = 0;
		  	  PERIOD_BEGIN(&audio_latency);
		  }
		  for (register int i = 0; i < SAMPLES_TO_FRAMES(AUDIO_SAMPLES_COUNT); i++) {
			for (register int j = 0; j < CHANNELS; j++) {
			  sampleData[j] = sample_io[2u * i + j];
			}
			sample.sampleData = sampleData;
			write_wav_sample(wav_in, &sample);
		  }
		  sample_count += SAMPLES_TO_FRAMES(AUDIO_SAMPLES_COUNT);
		  if (sample_count > RATE * DEMO_TIME) {
			fclose(wav_in);
			break;
		  }
		#else
		// idle time (purpose: reduce consumption)
		sleep_ms(BUFFER_LATENCY);
		#endif
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

	#if (DEMO == 1)
	  FILE* wav_out = fopen("demo_output.wav","wb");
	  int sample_count = 0;
	  int first_buffer = 1;
	  write_wav_header(wav_out, CHANNELS, RATE, 2, DEMO_TIME * RATE);
	  wav_sample_t sample = {
        CHANNELS,
        2,
        2 * CHANNELS,
        NULL
      };
	#endif

	timed_buffer_t play_buf;

	char victime[RAW_AUDIO_BUFFER_SIZE];
	play_buf.data = victime;

	long chunk_play_count = 0;

	// set RR realtime prio
	setscheduler(20);

    int ok = 1;

	debug("thread_playback_audio: running...");
    while (ok) {
		#ifdef __THRD_PARTY_PIPES__
		while(pipe_pop(play, play_buf.data, AUDIO_SAMPLES_COUNT))
		#else // __LINUX_PIPES__
		while(read(play, play_buf.data, SAMPLES_COUNT) > 0)
		#endif
		{
			#ifdef __DEBUG__
			  PERIOD_BEGIN(&play_buf.period);
			#endif
			if (playback_write(play_buf.data, SAMPLES_TO_FRAMES(AUDIO_SAMPLES_COUNT)) < 0) {
				ok = 0; break;
			}
			++chunk_play_count;
			#ifdef __DEBUG__
			  PERIOD_FINISH(&play_buf.period);
			  debug("%u = chunk[%lu] played  { from %lfs to %lfs in %lfs }\n",
			  	(unsigned int)AUDIO_SAMPLES_COUNT, chunk_play_count,
				PERIOD_START(&play_buf.period), PERIOD_END(&play_buf.period),
				PERIOD_LEN(&play_buf.period));
			  // if(chunk_play_count == 1000) { ok = 0; break; }
			#endif
			#if(DEMO == 1)
			  if (sample_count > RATE * DEMO_TIME) {
			    fclose(wav_out);
			    error("DEMO: END!");
			  }
			  int16_t *sample_io = play_buf.data;
			  int16_t sampleData[CHANNELS] = {0};
			  if (first_buffer) {
				  first_buffer = 0;
				  PERIOD_FINISH(&audio_latency);
				  audio_skiped = (int)(RATE * PERIOD_LEN(&audio_latency));
				  for (register int i = 0; i < audio_skiped; i++) {
					sample.sampleData = sampleData;
					write_wav_sample(wav_out, &sample);
				  }
				  sample_count += audio_skiped;
			  }
			  for (register int i = 0; i < SAMPLES_TO_FRAMES(AUDIO_SAMPLES_COUNT); i++) {
				for (register int j = 0; j < CHANNELS; j++) {
				  sampleData[j] = sample_io[2u * i + j];
				}
				sample.sampleData = sampleData;
				write_wav_sample(wav_out, &sample);
			  }
			  sample_count += SAMPLES_TO_FRAMES(AUDIO_SAMPLES_COUNT);
			#else
			// idle time (purpose: reduce consumption)
			sleep_ms(BUFFER_LATENCY);
			#endif
		}
    }
	pthread_exit(NULL);
}

// direction of arrival
doa_t target = {
    .detected = 0,
    .theta = {
        DOA_NOT_INITIALISED,
        DOA_NOT_INITIALISED,
        DOA_NOT_INITIALISED
    }
};
m_init(mutex_doa);
#define DOA_RESET(pdoa) def( \
    (pdoa)->detected = 0; \
    (pdoa)->theta[0] = DOA_NOT_INITIALISED; \
    (pdoa)->theta[1] = DOA_NOT_INITIALISED; \
    (pdoa)->theta[2] = DOA_NOT_INITIALISED)
#define DOA_READ(pdoa) secured_stuff(mutex_doa, *(pdoa) = target)
#define DOA_WRITE(detected, theta) secured_stuff(mutex_doa, \
    target.detected = detected; target.theta[0] = theta[0]; \
    target.theta[1] = theta[1]; target.theta[2] = theta[2])

void* thread_openCV(void* parameters) {

	debug("thread_openCV: init...");

	#ifdef __THRD_PARTY_PIPES__
	pipe_producer_t* doa_flow = (pipe_producer_t*)parameters;
	#else // __LINUX_PIPES__
	int doa_flow = *(int *)parameters;
	debug("doa_flow = %d", doa_flow);
	#endif

	// direction of arrival
    doa_t* ptarget = &target;

	for (;;) {
		/* code */
		sleep_ms(100);
	}

	pthread_exit(NULL);
}

#ifndef __arm__
  #define MAX_WORKERS 5
#else // __arm__
  #define MAX_WORKERS 10
#endif

#define POOL_IDLE_COUNT(thpool) MAX_WORKERS - thpool_num_threads_working(thpool)

struct container2 {
    pipe_bridge_t bridge;
    pipe_consumer_t* doa;
    pipe_consumer_t* prev;
    pipe_producer_t* current;
};

void* thread_fdbm_pool(void* parameters) {

	debug("thread_fdbm_pool: init...");
	threadpool fdbm_pool = thpool_init(MAX_WORKERS);
	int idle_threads = POOL_IDLE_COUNT(fdbm_pool);

	// more pipes
	pipe_t* pipe_overlap_win = pipe_new(SAMPLE_BYTES, AUDIO_SAMPLES_COUNT * AUDIO_CHUNKS/2);
	pipe_producer_t* current = pipe_producer_new(pipe_overlap_win);
	pipe_consumer_t* prev = pipe_consumer_new(pipe_overlap_win);

	// init
	int16_t zeros[AUDIO_SAMPLES_COUNT];
    // int16_t test[AUDIO_SAMPLES_COUNT];
	pipe_push(current, zeros, AUDIO_SAMPLES_COUNT);
    // pipe_pop(prev, test, AUDIO_SAMPLES_COUNT);

	// parameters (this part should be changed later!)
    struct container1* params = (struct container1*)parameters;
    struct container2* shin_params = malloc(sizeof(struct container2));
    shin_params->bridge = params->bridge;
    shin_params->doa = params->doa;
    shin_params->prev = prev;
    shin_params->current = current;

	debug("thread_fdbm_pool: running...");
	for(;;) {

		// fdbm_worker(shin_params);
		debug("POOL: %d IDLE threads out of %d !", idle_threads, (int)MAX_WORKERS);
		if (idle_threads > 0) {
			thpool_add_work(fdbm_pool, fdbm_worker, shin_params);
			sleep_ms(BUFFER_LATENCY / 4);
		} else {
			// delaying the workers to ensure the FIFO qualification of the pipes
			sleep_ms(3 * BUFFER_LATENCY / 4);
		}
		idle_threads = POOL_IDLE_COUNT(fdbm_pool);
	}

	// free pipes
	pipe_producer_free(current);
	pipe_consumer_free(prev);
	pipe_free(pipe_overlap_win);

	// free the pool
	thpool_wait(fdbm_pool);
	thpool_destroy(fdbm_pool);

	pthread_exit(NULL);
}

int global_fdbm_running = 0;
m_init(mutex_fdbm);

void fdbm_worker(void* parameters) {
	// set RR realtime prio
	setscheduler(15);

	// local routine count
	int local_fdbm_running;
	secured_stuff(mutex_fdbm, local_fdbm_running = ++global_fdbm_running);
	debug("thread_fdbm(%d): init...", local_fdbm_running);

	// local variables (from parameters)
    struct container2* params = (struct container2*)parameters;
	pipe_bridge_t bridge = params->bridge;
	pipe_consumer_t* doa_queue = params->doa;
	pipe_consumer_t* prev = params->prev;
	pipe_producer_t* current = params->current;
    // local variables (data)
    overlap_buffer_t buffer;
	int16_t* capture = &buffer;
	int16_t prev_fdbm[AUDIO_SAMPLES_COUNT];
	int16_t playback[AUDIO_SAMPLES_COUNT];
	debug("thread_fdbm(%d): buffer allocated", local_fdbm_running);

	// pull raw audio
	#ifdef __THRD_PARTY_PIPES__
	if(pipe_pop(bridge.from, capture, FDBM_SAMPLES_COUNT)) {
	#else // __LINUX_PIPES__
	while(read(bridge.from, buffer, SAMPLES_COUNT) < 0);
	#endif

	// if(pipe_pop(prev, prev_fdbm, AUDIO_SAMPLES_COUNT)) {
    pipe_pop(prev, prev_fdbm, AUDIO_SAMPLES_COUNT);
		// algorithms
		debug("thread_fdbm(%d): running...", local_fdbm_running);
		// applyFDBM_simple1(capture, FDBM_SAMPLES_COUNT, manual_doa);

		pipe_push(current, capture + AUDIO_SAMPLES_COUNT, AUDIO_SAMPLES_COUNT);
		for (size_t i = 0; i < AUDIO_SAMPLES_COUNT; i++) {
			playback[i] = prev_fdbm[i] + capture[i];
		}
	// }

	// push processed audio
	#ifdef __THRD_PARTY_PIPES__
	pipe_push(bridge.to, playback, AUDIO_SAMPLES_COUNT);
	}
	#else // __LINUX_PIPES__
	write(bridge.to, buffer, SAMPLES_COUNT);
	#endif

	debug("thread_fdbm(%d): Done!\n", local_fdbm_running);
}
