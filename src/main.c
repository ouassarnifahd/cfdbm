#include "common.h"
#include "threads.h"

int main(int argc, char const *argv[]) {
    init_log();
    sleep(1);
    debug("This is a 1 sec delayed test MSG");

    log_printf("Setting priority...\n");
    // setscheduler();

    log_printf("FBDM starting...\n");
    threads_init();

    return EXIT_SUCCESS;
}
