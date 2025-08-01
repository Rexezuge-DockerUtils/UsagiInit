#pragma once

#include <sys/types.h>

#define MAX_ARGS 128

typedef struct {
  pid_t pid;
  char *args[MAX_ARGS];
  int restart_count;
} Service;

void add_service(pid_t pid, char **args);
Service *find_service(pid_t pid);
void remove_service(pid_t pid);
