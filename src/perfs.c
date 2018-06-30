#include "common.h"

unsigned long get_cyclecount(void) {
    // unsigned int value;
    // Read CCNT Register
    // asm volatile ("MRC p15, 0, %0, c9, c13, 0\n": "=r"(value));
    // asm volatile ("mcr p15, 0, %0, c15, c9, 0\n": : "r" (1));

    unsigned int pmccntr;
    unsigned int pmuseren;
    unsigned int pmcntenset;
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

unsigned long get_cyclediff(unsigned long tsc1, unsigned long tsc2) {
    return tsc2 - tsc1;
}

void setscheduler(void) {
    struct sched_param sched_param;
    if (sched_getparam(0, &sched_param) < 0) {
        warning("Scheduler getparam failed...\n");
        return;
    }
    sched_param.sched_priority = sched_get_priority_max(SCHED_RR);
    if (!sched_setscheduler(0, SCHED_RR, &sched_param)) {
        debug("Scheduler set to Round Robin with priority %i...", sched_param.sched_priority);
        fflush(stdout);
        return;
    }
    warning("!!!Scheduler set to Round Robin with priority %i FAILED!!!", sched_param.sched_priority);
}
