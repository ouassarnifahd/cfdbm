#include "common.h"
#include "threads.h"

int main(int argc, char const *argv[]) {
    init_log();
    debug("Starting Stamp = %lu", tsc_run);
    log_printf("Setting priority...\n");
    unsigned long tsc1 = get_cyclecount();
    debug("stamp1 = %lu", tsc1);
    #ifdef __DEBUG__
    sleep(1);
    #endif
    unsigned long tsc2 = get_cyclecount();
    debug("stamp2 = %lu", tsc2);
    debug("stamps_diff = %lu", get_cyclediff(tsc1, tsc2));
    debug("CYCLE_TIME = %lf", (double)CYCLE_TIME_S);
    debug("This is a 1s (= %lfs) delayed test MSG", get_timediff(tsc1, tsc2));
    // setscheduler();

    log_printf("FBDM starting...\n");
    // threads_init();

    return EXIT_SUCCESS;
}
