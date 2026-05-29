#pragma once

#include <sys/types.h>
#include <time.h>

#define MAX_ARGS 128

typedef struct {
  pid_t   pid;
  char   *args[MAX_ARGS];
  char  **env;
  int     restart_count;
  time_t  next_restart;
} Service;

void     add_service(pid_t pid, char **args, char **env);
Service *find_service(pid_t pid);
void     remove_service(pid_t pid);
int      get_service_count();
time_t   get_next_restart_time(void);
Service *get_service_pending_restart(time_t now);
