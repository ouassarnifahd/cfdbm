#include "common.h"

void setscheduler(int prio) {
    struct sched_param sched_param;
    if (sched_getparam(0, &sched_param) < 0) {
        warning("Scheduler getparam failed...\n");
        return;
    }
    sched_param.sched_priority = prio; // sched_get_priority_max(SCHED_RR);
    if (!sched_setscheduler(0, SCHED_RR, &sched_param)) {
        debug("Scheduler set to Round Robin with priority %i...", sched_param.sched_priority);
        fflush(stdout);
        return;
    }
    warning("!!!Scheduler set to Round Robin with priority %i FAILED!!!", sched_param.sched_priority);
}
