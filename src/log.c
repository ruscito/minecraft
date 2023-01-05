#include "log.h"

#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef NDEBUG
  static log_level_t log_level = ERROR;
#else
  static log_level_t log_level = TRACE;
#endif

static char *level_string[6] = {
  "[TRACE] ",
  "[DEBUG] ",
  "[INFO] ",
  "[WARNING]",
  "[ERROR] ",
  "[FATAL] ",
};

void console_write(const char *message, char color){
    // ref: https://stackoverflow.com/questions/33309136/change-color-in-os-x-console-output
    const char *message_color[] = {"\x1b[36m",
        "\x1b[35m",
        "\x1b[32m",
        "\x1b[33m",
        "\x1b[31m",
        "\x1b[37;41m"};
    printf("%s%s\x1b[0m",message_color[color], message);
}

static int _string_format(char* dest, const char *fmt, void *va_list) {
  char buffer[32000];
  int n = vsnprintf(buffer, 32000, fmt, va_list);
  buffer[n] = 0;
  memcpy(dest, buffer, n+1);
  return n;
}

int string_format(char* dest, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int n = _string_format(dest, fmt, args);
  va_end(args);
  return n;
}


void set_log_level(log_level_t level) {
  if (level > 5) {
    log_level = 5; 
  } else {
    log_level = level;
  }
}

void log_output(log_level_t level, const char *fmt, ...){
  // calculate time stamp
  if (level < log_level) {
    return;
  }
  char time_stamp[9];
  char msg[4096];
  strftime(time_stamp, sizeof(time_stamp), "%T", localtime(&(time_t){time(NULL)}));
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg),fmt, args);
  char msg_out[32000];
  string_format(msg_out, "%s %s%s\n", time_stamp, level_string[level],msg);
  va_end(args);
  
  console_write(msg_out, level);
}

