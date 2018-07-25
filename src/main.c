#ifdef __DEBUG__
#define __MAIN__
#endif
#include "common.h"
#include "threads.h"

int main(int argc, char const *argv[]) {
    init_log();
    log_printf("FBDM starting...\n");
    log_printf("RT priority...   ");
    setscheduler(5);
    log_printf(" [ OK ]\n");
    threads_init();
    debug("FY!");
    return EXIT_SUCCESS;
}
