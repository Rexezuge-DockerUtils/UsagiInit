#pragma once

// ANSI color codes
#define COLOR_RESET "\x1b[0m"
#define COLOR_INFO "\x1b[32m"  // Green
#define COLOR_WARN "\x1b[33m"  // Yellow
#define COLOR_ERROR "\x1b[31m" // Red
#define COLOR_TRACE "\x1b[36m" // Cyan
#define COLOR_DEBUG "\x1b[35m" // Magenta

// Logging macros
#define LOG_INFO(fmt, ...)                                                     \
  log_message("INFO", COLOR_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)                                                     \
  log_message("WARN", COLOR_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)                                                    \
  log_message("ERROR", COLOR_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_TRACE(fmt, ...)                                                    \
  log_message("TRACE", COLOR_TRACE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)                                                    \
  log_message("DEBUG", COLOR_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

// Function prototype
void log_message(const char *level, const char *color, const char *file,
                 int line, const char *fmt, ...);
