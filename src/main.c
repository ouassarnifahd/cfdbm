#include "common.h"
#include "threads.h"

int main(int argc, char const *argv[]) {
    init_log();
    log_printf("Setting priority...\n");
    #ifdef __DEBUGED__
    sleep(1);
    debug("This is a 1s delayed test MSG");
    #endif
    setscheduler();

    log_printf("FBDM starting...\n");
    threads_init();

    return EXIT_SUCCESS;
}
