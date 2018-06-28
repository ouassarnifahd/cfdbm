#include "common.h"
#include "threads.h"

int main(int argc, char const *argv[]) {
    init_log();
    log_printf("FBDM starting...\n");

    setscheduler();

    threads_init();

    return EXIT_SUCCESS;
}
