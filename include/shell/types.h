#pragma once

// Struct to represent a command and its redirections
typedef struct {
  const char *executable; // Path to executable (e.g., "./a.out")
  char *const *args; // Argument list, NULL-terminated (args[0] = executable)
  const char *stdin_path;  // Optional input redirection (NULL if not used)
  const char *stdout_path; // Optional output redirection (NULL if not used)
  const char *stderr_path; // Optional error redirection (NULL if not used)
  int append_stdout; // 1 if stdout should be appended (>>), 0 for overwrite (>)
  int append_stderr; // 1 if stderr should be appended, 0 for overwrite
} command_t;
