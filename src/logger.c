#include "logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void log_message(const char *level, const char *color, const char *file,
                 int line, const char *fmt, ...) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  char timestamp[20];

  if (t != NULL) {
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
  } else {
    snprintf(timestamp, sizeof(timestamp), "unknown-time");
  }

  // Print formatted log message
#ifdef APP_NAME
  fprintf(stdout, "%s[%s] %s (%s): ", color, level, timestamp, APP_NAME);
#else
  fprintf(stdout, "%s[%s] %s (%s:%d): ", color, level, timestamp, file, line);
#endif

  va_list args;
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);

  fprintf(stdout, "%s\n", COLOR_RESET); // Reset color and newline
  fflush(stdout);
}
