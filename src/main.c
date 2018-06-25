#include "common.h"
#include "threads.h"

int main(int argc, char const *argv[]) {
    init_log();
    debug("FBDM starting...");

    // setscheduler();

    threads_init();

    return EXIT_SUCCESS;
}
