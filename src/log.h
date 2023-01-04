#pragma once

#pragma once

typedef enum log_level
{
    TRACE   = 0,
    DEBUG   = 1,
    INFO    = 2,
    WARNING = 3,
    ERROR   = 4,
    FATAL   = 5
} log_level_t;

#define  TRACE(fmt, ...)     log_output(TRACE,   fmt, ##__VA_ARGS__);
#define  DEBUG(fmt, ...)     log_output(DEBUG,   fmt, ##__VA_ARGS__);
#define  INFO(fmt, ...)      log_output(INFO,    fmt, ##__VA_ARGS__);
#define  WARNING(fmt, ...)   log_output(WARNING, fmt, ##__VA_ARGS__);
#define  ERROR(fmt, ...)     log_output(ERROR,   fmt, ##__VA_ARGS__);
#define  FATAL(fmt, ...)     log_output(FATAL,   fmt, ##__VA_ARGS__);

void log_output(log_level_t level, const char *fmt, ...);

void set_log_level(log_level_t level);



