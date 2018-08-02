#ifdef __DEBUG__
#define __MAIN__
#endif
#include "common.h"
#include "threads.h"

// #define OFFLINE_FDBM

extern char* wave_file;
extern int manual_doa;

int main(int argc, char const *argv[]) {
    init_log();
    log_printf("FBDM starting...\n");
    log_printf("RT priority...   ");
    setscheduler(5);
    log_printf(" [ OK ]\n");
    #ifdef OFFLINE_FDBM
    if (argc == 2) {
        strncpy(wave_file, argv[1], strlen(argv[1]));
    } else {
        printf("usage: %s <wave_file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    #endif
    if (argc == 2) {
        manual_doa = atoi(argv[1]);
    } else {
        printf("usage: %s <doa>\n", argv[0]);
        return EXIT_FAILURE;
    }
    threads_init();
    debug("FY!");
    return EXIT_SUCCESS;
}
