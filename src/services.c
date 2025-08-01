#include "services.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/limits.h>

#define MAX_SERVICES 100

static Service services[MAX_SERVICES];
static int service_count = 0;

void add_service(pid_t pid, char **args) {
  if (service_count >= MAX_SERVICES) {
    LOG_WARN("Maximum number of services reached.");
    return;
  }

  services[service_count].pid = pid;
  int i = 0;

  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    LOG_ERROR("getcwd() error");
    return;
  }

  char path[PATH_MAX];
  if (args[0][0] == '/') {
    strncpy(path, args[0], sizeof(path));
  } else {
    snprintf(path, sizeof(path), "%s/%s", cwd, args[0]);
  }
  
  services[service_count].args[0] = strdup(path);

  for (i = 1; args[i] != NULL; i++) {
    services[service_count].args[i] = strdup(args[i]);
  }
  services[service_count].args[i] = NULL;
  services[service_count].restart_count = 0;
  service_count++;
  LOG_INFO("Service added (PID: %d).", pid);
}

Service *find_service(pid_t pid) {
  for (int i = 0; i < service_count; i++) {
    if (services[i].pid == pid) {
      return &services[i];
    }
  }
  return NULL;
}

void remove_service(pid_t pid) {
  for (int i = 0; i < service_count; i++) {
    if (services[i].pid == pid) {
      // Free the duplicated strings
      for (int j = 0; services[i].args[j] != NULL; j++) {
        free(services[i].args[j]);
      }
      // Shift the remaining services
      for (int j = i; j < service_count - 1; j++) {
        services[j] = services[j + 1];
      }
      service_count--;
      LOG_INFO("Service removed (PID: %d).", pid);
      return;
    }
  }
}
