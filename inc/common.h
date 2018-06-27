#ifndef __HEADER_COMMON__
#define __HEADER_COMMON__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <sys/time.h>
#include <math.h>

#include "error.h"
#include "preprocessor.h"

//perf
inline unsigned long get_cyclecount(void);
inline unsigned long get_cyclediff(unsigned long tsc1, unsigned long tsc2);

void setscheduler(void);

#endif /* end of include guard: __HEADER_COMMON__ */
