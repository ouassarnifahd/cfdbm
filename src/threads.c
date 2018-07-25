// #undef __DEBUG__
#include "common.h"
#include "fdbm.h"
#include "threads.h"
#include "pipe.h"
#include "thpool.h"
#include "wav.h"

// demo flags
#define DEMO 1

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

#define BUFFER_LATENCY 25

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
	log_printf("PIPES init...    ");
	// THE Bridge
	void* fdbm_bridge = malloc(3 * sizeof(pipe_generic_t*));

	#ifdef __USE_ALSA__
	  int frame_bytes = 4;//input->buffer.bytes_per_frame;
	#else // __USE_PULSEAUDIO__
	  int frame_bytes = 4;
	#endif

	#ifdef __THRD_PARTY_PIPES__
	  pipe_t* pipe_into_fdbm = pipe_new(frame_bytes, SAMPLES_COUNT * BUFFER_CHUNKS);
	  pipe_t* pipe_from_fdbm = pipe_new(frame_bytes, SAMPLES_COUNT * BUFFER_CHUNKS);
	  pipe_t* pipe_fdbm_doa  = pipe_new(sizeof(int), DOA_BUFFER);

	  // First STEP
	  pipe_producer_t* pipe_audio_in = pipe_producer_new(pipe_into_fdbm);
	  pipe_consumer_t* pipe_fdbm_in  = pipe_consumer_new(pipe_into_fdbm);

	  // Second STEP
	  pipe_producer_t* pipe_fdbm_out  = pipe_producer_new(pipe_from_fdbm);
	  pipe_consumer_t* pipe_audio_out = pipe_consumer_new(pipe_from_fdbm);

	  // Third STEP
	  pipe_producer_t* pipe_doa_in  = pipe_producer_new(pipe_fdbm_doa);
	  pipe_consumer_t* pipe_doa_out = pipe_consumer_new(pipe_fdbm_doa);

	  // THE Bridge
	  pipe_bridge_t* bridge = fdbm_bridge;
	  char* pipes = fdbm_bridge;
	  bridge->from = pipe_fdbm_in;
	  bridge->to   = pipe_fdbm_out;
	  memcpy(pipes + sizeof(pipe_bridge_t), pipe_doa_out, sizeof(pipe_consumer_t*));
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
	  int** pipes = fdbm_bridge;
	  bridge->from = *pipe_fdbm_in;
	  bridge->to   = *pipe_fdbm_out;
	  // pipes[2]     = *pipe_fdbm_doa;
	#endif // __THRD_PARTY_PIPES__

	log_printf(" [ OK ]\n");

	// AUDIO init...
	log_printf("AUDIO init...    ");
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

	log_printf("AUDIO Capture... ");
	attach_to_core(&attr_capture, audioIO_CORE);
	if (pthread_create(&audio_capture_process, &attr_capture, thread_capture_audio, pipe_audio_in)) {
		error("audio_capture_process init failed"); perror(NULL);
	}
	pthread_setname_np(audio_capture_process, "CFDBM capture");
	log_printf(" [ ON ]\n");

	log_printf("FDBM   worker... ");
	attach_to_core(&attr_fdbm, audioProcessingCORE);
	if(pthread_create(&fdbm_process, &attr_fdbm, thread_fdbm_pool, fdbm_bridge)) {
			error("fdbm_process init failed"); perror(NULL);
		}
	pthread_setname_np(fdbm_process, "CFDBM pool");
	log_printf(" [ ON ]\n");

	// OpenCV init...
	log_printf("OpenCV worker... ");
	attach_to_core(&attr_openCV, audioProcessingCORE);
	if(pthread_create(&openCV_process, &attr_openCV, thread_openCV, pipe_doa_in)) {
			error("fdbm_process init failed"); perror(NULL);
	}
	pthread_setname_np(openCV_process, "CFDBM camera");
	log_printf(" [ ON ]\n");
	// SIGHANDLING interface

	// The main core stops here and waits for all the others!!
	pthread_join(openCV_process, NULL);
	pthread_join(fdbm_process, NULL);
	pthread_join(audio_capture_process, NULL);
	pthread_join(audio_playback_process, NULL);

	// Return what you took! dont carry that weight!
	pthread_attr_destroy(&attr_capture);
	pthread_attr_destroy(&attr_playback);
	pthread_attr_destroy(&attr_fdbm);
	pthread_attr_destroy(&attr_openCV);

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

	char buf[RAW_AUDIO_BUFFER_SIZE];
	capt_buf.data = buf;

	long chunk_capture_count = 0;

	// set RR realtime prio
	setscheduler(20);

    int ok = 1;

	debug("thread_capture_audio: running...");
	while (ok) {
		#ifdef __DEBUG__
		  PERIOD_BEGIN(&capt_buf.period);
		#endif
		if (capture_read(capt_buf.data, SAMPLES_COUNT) < 0) ok = 0;
		++chunk_capture_count;
		#ifdef __DEBUG__
		  PERIOD_FINISH(&capt_buf.period);
		  debug("%u = chunk[%lu] captured { from %lfs to %lfs in %lfs }\n",
		  	(unsigned int)SAMPLES_COUNT, chunk_capture_count,
			PERIOD_START(&capt_buf.period), PERIOD_END(&capt_buf.period),
			PERIOD_LEN(&capt_buf.period));
		  // if(chunk_capture_count == 1000) break;
		#endif
		#ifdef __THRD_PARTY_PIPES__
		pipe_push(capture, capt_buf.data, SAMPLES_COUNT);
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
		  for (register int i = 0; i < SAMPLES_COUNT; i++) {
			for (register int j = 0; j < CHANNELS; j++) {
			  sampleData[j] = sample_io[2u * i + j];
			}
			sample.sampleData = sampleData;
			write_wav_sample(wav_in, &sample);
		  }
		  sample_count += SAMPLES_COUNT;
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
		while(pipe_pop(play, play_buf.data, SAMPLES_COUNT))
		#else // __LINUX_PIPES__
		while(read(play, play_buf.data, SAMPLES_COUNT) > 0)
		#endif
		{
			#ifdef __DEBUG__
			  PERIOD_BEGIN(&play_buf.period);
			#endif
			if (playback_write(play_buf.data, SAMPLES_COUNT) < 0) {
				ok = 0; break;
			}
			++chunk_play_count;
			#ifdef __DEBUG__
			  PERIOD_FINISH(&play_buf.period);
			  debug("%u = chunk[%lu] played  { from %lfs to %lfs in %lfs }\n",
			  	(unsigned int)SAMPLES_COUNT, chunk_play_count,
				PERIOD_START(&play_buf.period), PERIOD_END(&play_buf.period),
				PERIOD_LEN(&play_buf.period));
			  // if(chunk_play_count == 1000) { ok = 0; break; }
			#endif
			#if(DEMO == 1)
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
			  for (register int i = 0; i < SAMPLES_COUNT; i++) {
				for (register int j = 0; j < CHANNELS; j++) {
				  sampleData[j] = sample_io[2u * i + j];
				}
				sample.sampleData = sampleData;
				write_wav_sample(wav_out, &sample);
			  }
			  sample_count += SAMPLES_COUNT;
			  if (sample_count > RATE * DEMO_TIME) {
				fclose(wav_out);
				error("DEMO: END!");
			  }
			#else
			// idle time (purpose: reduce consumption)
			sleep_ms(BUFFER_LATENCY);
			#endif
		}
    }
	pthread_exit(NULL);
}

void* thread_openCV(void* parameters) {

	debug("thread_openCV: init...");
	// log_printf("CAPTURE Process is running...\n");

	#ifdef __THRD_PARTY_PIPES__
	pipe_producer_t* doa_flow = (pipe_producer_t*)parameters;
	#else // __LINUX_PIPES__
	int doa_flow = *(int *)parameters;
	debug("doa_flow = %d", doa_flow);
	#endif

	for (;;) {
		/* code */
		sleep_ms(100);
	}

	pthread_exit(NULL);
}

#define MAX_WORKERS 2

#define POOL_IDLE_COUNT(thpool) MAX_WORKERS - thpool_num_threads_working(thpool)

void* thread_fdbm_pool(void* parameters) {

	debug("thread_fdbm_pool: init...");
	threadpool fdbm_pool = thpool_init(MAX_WORKERS);
	int idle_threads = POOL_IDLE_COUNT(fdbm_pool);

	debug("thread_fdbm_pool: running...");
	for(;;)
	{
		debug("POOL: %d IDLE threads out of %d !", idle_threads, (int)MAX_WORKERS);
		if (POOL_IDLE_COUNT(fdbm_pool) > 0) {
			thpool_add_work(fdbm_pool, thread_fdbm_worker, parameters);
		}
		// delaying the workers to ensure the FIFO quality of the pipe
		sleep_ms(3 * BUFFER_LATENCY / 4);
		idle_threads = POOL_IDLE_COUNT(fdbm_pool);
	}

	// free the pool
	thpool_wait(fdbm_pool);
	thpool_destroy(fdbm_pool);

	pthread_exit(NULL);
}

int global_fdbm_running = 0;
m_init(mutex_fdbm);

void* thread_fdbm_worker(void* parameters) {
	// set RR realtime prio
	setscheduler(15);

	// local routine count
	int local_fdbm_running;
	secured_stuff(mutex_fdbm, local_fdbm_running = ++global_fdbm_running);
	debug("thread_fdbm(%d): init...", local_fdbm_running);

	// local variables
	pipe_bridge_t bridge = *(pipe_bridge_t*)parameters;
	char* buffer = malloc(RAW_FDBM_BUFFER_SIZE);
	debug("thread_fdbm(%d): buffer allocated", local_fdbm_running);

	// pull raw audio
	#ifdef __THRD_PARTY_PIPES__
	if(pipe_pop(bridge.from, buffer, SAMPLES_COUNT)) {
	#else // __LINUX_PIPES__
	while(read(bridge.from, buffer, SAMPLES_COUNT) < 0);
	#endif

	// algorithms
	debug("thread_fdbm(%d): running...", local_fdbm_running);
	applyFDBM_simple1(buffer, SAMPLES_COUNT, DOA_CENTER);

	// push processed audio
	#ifdef __THRD_PARTY_PIPES__
	pipe_push(bridge.to, buffer, SAMPLES_COUNT); }
	#else // __LINUX_PIPES__
	write(bridge.to, buffer, SAMPLES_COUNT);
	#endif

	free(buffer);
	debug("thread_fdbm(%d): Done!\n", local_fdbm_running);
}
