#ifndef __HEADER_COMMON__
#define __HEADER_COMMON__

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <errno.h>
#include <time.h>
#include <math.h>

// you cant see me!!
#define INVISIBLE static inline __attribute__((always_inline))

// synchronization (mutex)
#define m_init(mutex) pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER

#define m_lock(mutex) pthread_mutex_lock(mutex)
#define m_unlock(mutex) pthread_mutex_unlock(mutex)

#define secured_stuff(mutex, stuff) do { \
    m_lock(&mutex);          \
    do { stuff; } while(0);  \
    m_unlock(&mutex);        \
 } while(0)

// timeee
#define NANO_SECOND_MULTIPLIER 1000000L
#define sleep_ms(ms) nanosleep((const struct timespec[]){{0, (ms * NANO_SECOND_MULTIPLIER)}}, NULL)

#include "preprocessor.h"

#define __DEBUG_LOG_TIMESTAMP__
#define __DEBUG_LOG_THREADS__
#include "error.h"

#ifndef __NO_NEON__
#include "neon_wrapper.h"
#endif

// #define __USE_ALSA__
#include "audio_wrapper.h"

// #include "opencv_wrapper.h"

void setscheduler(int prio);

#endif /* end of include guard: __HEADER_COMMON__ */
