#ifndef ERROR_H
#define ERROR_H

// #define __DEBUG_LOG_TIMESTAMP__
// #define __DEBUG_LOG_THREADS__

// #define INVISIBLE static inline __attribute__((always_inline))

#define LOG_BUFFER_SIZE 512

#define CLR_RED "\033[0;31m"
#define CLR_GRN "\033[0;32m"
#define CLR_YLW "\033[1;33m"
#define CLR_BLU "\033[0;34m"
#define CLR_PPL "\033[0;35m"
#define CLR_WIT "\033[0m"

char out_buf[LOG_BUFFER_SIZE];
char out_str[LOG_BUFFER_SIZE];

//perf
#define KHz (10000L)
#define MHz (10000000L)
#define GHz (1000000000L)
#define CPU_FREQUENCY (1*GHz)
#define CYCLE_TIME_S  (1/CPU_FREQUENCY)
#define CYCLE_TIME_MS (CYCLE_TIME_S * KHz)
#define CYCLE_TIME_NS (CYCLE_TIME_S * GHz)

#if defined (__x86_64__)
INVISIBLE unsigned long long rdtsc(void) {
    unsigned long hi = 0, lo = 0;
    asm volatile ("rdtsc" : "=a" (lo) , "=d" (hi));
    return (unsigned long long)hi << 32 | (unsigned long long)lo;
}
#elif defined (__i386__)
INVISIBLE unsigned long long rdtsc(void) {
    unsigned long x = 0;
    asm volatile ("rdtsc" : "=A" (x));
    return (unsigned long long)x;
}
#endif

INVISIBLE unsigned long get_cyclecount(void) {
    #if defined (__x86_64__) || defined (__i386__)
    // intel/amd timestamp assembly instruction rdtsc
    return (unsigned long)rdtsc();
    #elif defined (__arm__)
    register unsigned int pmccntr;
    register unsigned int pmuseren;
    register unsigned int pmcntenset;
    // Read the user mode perf monitor counter access permissions.
    asm volatile("mrc p15, 0, %0, c9, c14, 0" : "=r"(pmuseren));
    if (pmuseren & 1) {  // Allows reading perfmon counters for user mode code.
        asm volatile("mrc p15, 0, %0, c9, c12, 1" : "=r"(pmcntenset));
        if (pmcntenset & 0x80000000ul) {  // Is it counting?
            asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(pmccntr));
            // The counter is set up to count every 64th cycle
            return (unsigned long) pmccntr << 6;  // Should optimize to << 6
        }
    }
    return 1;
    #else
    static unsigned long long print_once = 0;
    if(!print_once++) warning("Incompatible architecture!");
    return 0;
    #endif
}

INVISIBLE unsigned long get_cyclediff(unsigned long tsc1, unsigned long tsc2) {
    return tsc2 - tsc1;
}

INVISIBLE double get_timediff(unsigned long tsc1, unsigned long tsc2) {
    return (double)get_cyclediff(tsc1, tsc2) * (double)CYCLE_TIME_S;
}

INVISIBLE double get_timediff_ns(unsigned long tsc1, unsigned long tsc2) {
    return (double)get_cyclediff(tsc1, tsc2) * (double)CYCLE_TIME_NS;
}

INVISIBLE double get_timediff_ms(unsigned long tsc1, unsigned long tsc2) {
    return (double)get_cyclediff(tsc1, tsc2) * (double)CYCLE_TIME_MS;
}

#define init_log()

#define log_printf(MSG, ...) {\
    sprintf(out_buf, MSG, ##__VA_ARGS__); \
    fprintf(stdout, "%s", out_buf); }

#define error(MSG, ...) {\
    int written_str = 0; \
    written_str += sprintf(out_str + written_str, CLR_RED"[ERROR]"CLR_WIT" (%s:%s:%i) ", __FILE__, __func__, __LINE__); \
    sprintf(out_str + written_str, MSG, ##__VA_ARGS__); fprintf(stderr, "%s\n", out_str); fflush(stderr); exit(0); }

#define warning(MSG, ...)

#define debug(MSG, ...)

#define alloc_check(PTR) { if((PTR) == NULL) { error("Out of Memory!"); } }

// TODO maybe next time
// extern const char BUILD_VERSION;
// fprintf(logfile, "# Build Version %lu\n", (unsigned long)BUILD_VERSION);

#ifdef __DEBUG__
  #include <time.h>

  #define LOGFILE_PATH "./debug/logfile.mlb"

  static FILE* logfile = NULL;
  char log_str[LOG_BUFFER_SIZE];

  #ifdef __DEBUG_LOG_TIMESTAMP__
    #define __TSC__ 1
    static unsigned long tsc_run;
    #define init_timestamp() { tsc_run = get_cyclecount(); }
    #define get_time_from_start() get_timediff(tsc_run, get_cyclecount())
  #else
    #define __TSC__ 0
  #endif // __DEBUG_LOG_TIMESTAMP__

  #ifdef __DEBUG_LOG_THREADS__
    #define __TRD__ 1
    #include <pthread.h>
    #include <sched.h>
    static int core_self = 0;
    static pthread_mutex_t lock_printf = PTHREAD_MUTEX_INITIALIZER;
    #define printf_lock() pthread_mutex_lock(&lock_printf)
    #define printf_unlock() pthread_mutex_unlock(&lock_printf)
  #else
    #define __TRD__ 0
  #endif // __DEBUG_LOG_THREADS__

  #define ctime_date(str_TIME, str) { \
    char* date_ = str_TIME + 4; \
    char* year_ = str_TIME + 20; \
    for (size_t i = 0; i < 7; i++) \
      str[i] = date_[i]; \
    for (size_t i = 0; i < 4; i++) \
      str[7 + i] = year_[i]; \
    str[12] = '\0'; \
  }

  #define ctime_time(str_TIME, str) { \
    char* time_ = str_TIME + 11; \
    for (size_t i = 0; i < 8; i++) \
      str[i] = time_[i]; \
    str[8] = '\0'; \
  }

  #undef init_log
  #define init_log() {\
    logfile = fopen(LOGFILE_PATH, "w"); \
    if (!logfile) { \
        fprintf(stderr, CLR_RED"[ERROR]"CLR_WIT" '%s' wont open!\n", LOGFILE_PATH); \
        exit(0); \
    } \
    time_t now; now = time(NULL); \
    char _date_[13]; char _time_[9]; ctime_date(ctime(&now), _date_); ctime_time(ctime(&now),_time_); \
    fprintf(logfile, "###########################################################\n"); \
    fprintf(logfile, "# Compiled on %s at %s\n", __DATE__, __TIME__); \
    fprintf(logfile, "# Executed on %s at %s\n", _date_, _time_); \
    fprintf(logfile, "###########################################################\n\n"); \
    fflush(logfile); fclose(logfile); logfile = NULL; \
    if (__TSC__ == 1) init_timestamp(); if (__TRD__ == 1) core_self = sched_getcpu(); }

  #define log(str) { \
    logfile = fopen(LOGFILE_PATH, "a"); \
    if (!logfile) { fprintf(stderr, CLR_RED"[ERROR]"CLR_WIT" '%s' wont open!\n", LOGFILE_PATH); exit(0); } \
    if(__TRD__ == 1) printf_lock(); fprintf(logfile, "%s\n", str); if(__TRD__ == 1) printf_unlock(); \
    fflush(logfile); fclose(logfile); logfile = NULL; }

  #undef error
  #define error(MSG, ...) { \
    int written_str = 0, written_log = 0; \
    if (__TRD__ == 1) printf_lock(); \
    written_str += sprintf(out_str + written_str, CLR_RED"[ ERROR ]"CLR_WIT" "); \
    written_log += sprintf(log_str + written_log, "[ ERROR ] "); \
    if (__TRD__ == 1) { \
      int self = pthread_self(); int core = sched_getcpu();\
      written_str += sprintf(out_str + written_str, CLR_BLU"{#%d@%d} "CLR_WIT, self, core); \
      written_log += sprintf(log_str + written_log, "{#%d@%d} ", self, core); } \
    if (__TSC__ == 1) { \
      double time_it_here = get_time_from_start(); \
      written_str += sprintf(out_str, "["CLR_GRN"%3.6lf"CLR_WIT"] ", time_it_here); \
      written_log += sprintf(log_str, "[%3.8lf] ", time_it_here); } \
    written_str += sprintf(out_str + written_str, "(%s:%s:%i) ", __FILE__, __func__, __LINE__); \
    written_log += sprintf(log_str + written_log, "(%s:%s:%i) ", __FILE__, __func__, __LINE__); \
    written_str += sprintf(out_str + written_str, MSG, ##__VA_ARGS__); \
    written_log += sprintf(log_str + written_log, MSG, ##__VA_ARGS__); \
    fprintf(stderr, "%s\n", out_str); if (__TRD__ == 1) printf_unlock(); \
    fflush(stderr); log(log_str); exit(0); }
  #undef warning
  #define warning(MSG, ...) { \
    int written_str = 0, written_log = 0; \
    if (__TRD__ == 1) printf_lock(); \
    written_str += sprintf(out_str + written_str, CLR_YLW"[WARNING]"CLR_WIT" "); \
    written_log += sprintf(log_str + written_log, "[WARNING] "); \
    if (__TRD__ == 1) { \
      int self = pthread_self(); int core = sched_getcpu();\
      written_str += sprintf(out_str + written_str, CLR_BLU"{#%d@%d} "CLR_WIT, self, core); \
      written_log += sprintf(log_str + written_log, "{#%d@%d} ", self, core); } \
    if (__TSC__ == 1) { \
      double time_it_here = get_time_from_start(); \
      written_str += sprintf(out_str, "["CLR_GRN"%3.6lf"CLR_WIT"] ", time_it_here); \
      written_log += sprintf(log_str, "[%3.8lf] ", time_it_here); } \
    written_str += sprintf(out_str + written_str, "(%s:%s:%i) ", __FILE__, __func__, __LINE__); \
    written_log += sprintf(log_str + written_log, "(%s:%s:%i) ", __FILE__, __func__, __LINE__); \
    written_str += sprintf(out_str + written_str, MSG, ##__VA_ARGS__); \
    written_log += sprintf(log_str + written_log, MSG, ##__VA_ARGS__); \
    fprintf(stdout, "%s\n", out_str); if (__TRD__ == 1) printf_unlock(); fflush(stdout); log(log_str); }
  #undef debug
  #define debug(MSG, ...) { \
    int written_str = 0, written_log = 0; \
    if (__TRD__ == 1) printf_lock(); \
    written_str += sprintf(out_str + written_str, CLR_PPL"[ DEBUG ]"CLR_WIT" "); \
    written_log += sprintf(log_str + written_log, "[ DEBUG ] "); \
    if (__TRD__ == 1) { \
      int self = pthread_self(); int core = sched_getcpu(); \
      written_str += sprintf(out_str + written_str, CLR_BLU"{#%d@%d} "CLR_WIT, self, core); \
      written_log += sprintf(log_str + written_log, "{#%d@%d} ", self, core); } \
    if (__TSC__ == 1) { \
      double time_it_here = get_time_from_start(); \
      written_str += sprintf(out_str + written_str, "["CLR_GRN"%3.6lf"CLR_WIT"] ", time_it_here); \
      written_log += sprintf(log_str + written_log, "[%3.8lf] ", time_it_here); } \
    written_str += sprintf(out_str + written_str, "(%s:%s:%i) ", __FILE__, __func__, __LINE__); \
    written_log += sprintf(log_str + written_log, "(%s:%s:%i) ", __FILE__, __func__, __LINE__); \
    written_str += sprintf(out_str + written_str, MSG, ##__VA_ARGS__); \
    written_log += sprintf(log_str + written_log, MSG, ##__VA_ARGS__); \
    fprintf(stdout, "%s\n", out_str); if (__TRD__ == 1) printf_unlock(); fflush(stdout); log(log_str); }

  #undef log_printf
  #define log_printf(MSG, ...) {\
    sprintf(log_str, MSG, ##__VA_ARGS__); if (__TRD__ == 1) printf_lock(); \
    fprintf(stdout, "%s", log_str); if (__TRD__ == 1) printf_unlock(); log(log_str); }
#endif // __DEBUG__

#endif /* end of include guard: ERROR_H */
