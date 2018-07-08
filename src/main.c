#ifdef __DEBUG__
#define __MAIN__
#endif
#include "common.h"
#include "threads.h"

int main(int argc, char const *argv[]) {
    init_log();
    #ifdef __DEBUGED__
    sleep_ms(100);
    debug("This is a 100ms delayed test MSG");
    #endif
    log_printf("Setting priority...\n");
    setscheduler(10);
    #ifdef __DEBUGED__
    sleep_ms(100);
    debug("This is a 100ms delayed test MSG");
    #endif
    log_printf("FBDM starting...\n");
    threads_init();
    debug("FY!");
    return EXIT_SUCCESS;
}
