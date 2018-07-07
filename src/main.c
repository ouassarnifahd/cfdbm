// for debuging purpose
#define __MAIN__
#include "common.h"
#include "threads.h"

int main(int argc, char const *argv[]) {
    init_log();
    #ifdef __DEBUG__
    sleep_ms(100);
    debug("This is a 100ms delayed test MSG");
    #endif
    log_printf("Setting priority...\n");
    setscheduler(20);
    #ifdef __DEBUG__
    sleep_ms(100);
    debug("This is a 100ms delayed test MSG");
    #endif
    log_printf("FBDM starting...\n");
    threads_init();

    debug("FY!");
    return EXIT_SUCCESS;
}
