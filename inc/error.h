#ifndef ERROR_H
#define ERROR_H

#define LOG_BUFFER_SIZE 128

#ifdef __DEBUG__
  #include <time.h>

  #define LOGFILE_PATH "./debug/logfile.mlb"

  char log_str[LOG_BUFFER_SIZE];
#endif

char out_buf[LOG_BUFFER_SIZE];
char out_str[LOG_BUFFER_SIZE];

#define init_log()

#ifdef __DEBUG__
  static FILE* logfile = NULL;

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
        fprintf(stderr, "\033[0;31m[ERROR]\033[0m '%s' wont open!\n", LOGFILE_PATH); \
        exit(0); \
    } \
    time_t now; now = time(NULL); \
    char _date_[13]; char _time_[9]; ctime_date(ctime(&now), _date_); ctime_time(ctime(&now),_time_); \
    fprintf(logfile, "###########################################################\n"); \
    fprintf(logfile, "# Compiled on %s at %s\n", __DATE__, __TIME__); \
    fprintf(logfile, "# Executed on %s at %s\n", _date_, _time_); \
    fprintf(logfile, "###########################################################\n\n"); \
    fflush(logfile); fclose(logfile); logfile = NULL; }

  #define log(str) {\
    logfile = fopen(LOGFILE_PATH, "a"); \
    if (!logfile) { \
        fprintf(stderr, "\033[0;31m[ERROR]\033[0m '%s' wont open!\n", LOGFILE_PATH); \
        exit(0); \
    } \
    fprintf(logfile, "%s\n", str); fflush(logfile); fclose(logfile); logfile = NULL; }
#endif

#define log_printf(MSG, ...) {\
    sprintf(out_buf, MSG, ##__VA_ARGS__); \
    fprintf(stdout, "%s", out_buf); }

#define error(MSG, ...) {\
    sprintf(out_str, "\033[0;31m[ERROR]\033[0m (%s:%s:%i) ", __FILE__, __func__, __LINE__); \
    sprintf(out_buf, MSG, ##__VA_ARGS__); strcat(out_str, out_buf); \
    fprintf(stderr, "%s\n", out_str); fflush(stderr); exit(0); }

#define warning(MSG, ...)

#define debug(MSG, ...)

#define alloc_check(PTR) { if((PTR) == NULL) { error("Out of Memory!"); } }

#ifdef __DEBUG__
    #undef error
    #define error(MSG, ...) {\
        sprintf(out_str, "\033[0;31m[ERROR]\033[0m (%s:%s:%i) ", __FILE__, __func__, __LINE__); \
        sprintf(log_str, "[ERROR] (%s:%s:%i) ", __FILE__, __func__, __LINE__); \
        sprintf(out_buf, MSG, ##__VA_ARGS__); strcat(out_str, out_buf); \
        strcat(log_str, out_buf); fprintf(stderr, "%s\n", out_str); \
        fflush(stderr); if(__DEBUG__) log(log_str); exit(0); }
    #undef warning
    #define warning(MSG, ...) {\
        sprintf(out_str, "\033[1;33m[WARNING]\033[0m (%s:%s:%i) ", __FILE__, __func__, __LINE__); \
        sprintf(log_str, "[WARNING] (%s:%s:%i) ", __FILE__, __func__, __LINE__); \
        sprintf(out_buf, MSG, ##__VA_ARGS__); strcat(out_str, out_buf); \
        strcat(log_str, out_buf); fprintf(stdout, "%s\n", out_str); \
        fflush(stdout); log(log_str); }
    #undef debug
    #define debug(MSG, ...) {\
        sprintf(out_str, "\033[0;35m[DEBUG]\033[0m (%s:%s:%i) ", __FILE__, __func__, __LINE__); \
        sprintf(log_str, "[DEBUG] (%s:%s:%i) ", __FILE__, __func__, __LINE__); \
        sprintf(out_buf, MSG, ##__VA_ARGS__); strcat(out_str, out_buf); \
        strcat(log_str, out_buf); fprintf(stdout, "%s\n", out_str); \
        fflush(stdout); log(log_str); }

    #undef Mat_printf
    #define Mat_printf(MSG, ...) {\
        sprintf(log_str, MSG, ##__VA_ARGS__); \
        fprintf(stdout, "%s", log_str); log(log_str); }
#endif

#endif /* end of include guard: ERROR_H */
