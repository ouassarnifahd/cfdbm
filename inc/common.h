#ifndef __HEADER_COMMON__
#define __HEADER_COMMON__

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <time.h>
#include <math.h>

// TODO !!!
#define __DEBUG_LOG_TIMESTAMP__
// #define __DEBUG_LOG_THREADS__
// #define __DEBUG_LOG_CORES__

#define INVISIBLE static inline __attribute__((always_inline))

#include "error.h"
#include "preprocessor.h"

#define NANO_SECOND_MULTIPLIER 1000000L
#define sleep_ms(ms) nanosleep((const struct timespec[]){{0, (ms * NANO_SECOND_MULTIPLIER)}}, NULL)

void setscheduler(void);

#endif /* end of include guard: __HEADER_COMMON__ */
