#ifndef __HEADER_COMMON__
#define __HEADER_COMMON__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <time.h>
#include <math.h>

#include "error.h"
#include "preprocessor.h"

//perf
#define CYCLE_TIME (1e-9)
#define CYCLE_TIME_MS (CYCLE_TIME * 1e6)

static inline __attribute__((always_inline)) unsigned long get_cyclecount(void) {
    register unsigned int pmccntr;
    register unsigned int pmuseren;
    register unsigned int pmcntenset;
    // Read the user mode perf monitor counter access permissions.
    asm volatile("mrc p15, 0, %0, c9, c14, 0" : "=r"(pmuseren));
    if (pmuseren & 1) {  // Allows reading perfmon counters for user mode code.
        asm volatile("mrc p15, 0, %0, c9, c12, 1" : "=r"(pmcntenset));
        if (pmcntenset & 0x80000000ul) {  // Is it counting?
            asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(pmccntr));
            // The counter is set up to count every 64th cycle
            return (unsigned long) pmccntr << 6;  // Should optimize to << 6
        }
    }
    return 1;
}

static inline __attribute__((always_inline)) unsigned long get_cyclediff(unsigned long tsc1, unsigned long tsc2) {
    return tsc2 - tsc1;
}

static inline __attribute__((always_inline)) double get_timediff_ns(unsigned long tsc1, unsigned long tsc2) {
    return (double)get_cyclediff(tsc1, tsc2) * (double)CYCLE_TIME;
}

static inline __attribute__((always_inline)) double get_timediff_ms(unsigned long tsc1, unsigned long tsc2) {
    return (double)get_cyclediff(tsc1, tsc2) * (double)CYCLE_TIME_MS;
}

void setscheduler(void);

#endif /* end of include guard: __HEADER_COMMON__ */
