#include "guardian.h"
#include "globals.h"
#include "logger.h"
#include "services.h"
#include "shell/utils.h"

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#ifdef RELEASE_MODE
#define MAX_RESTARTS 999
#else
#define MAX_RESTARTS 2
#endif

#define BACKOFF_BASE_SECS 1
#define BACKOFF_MAX_SECS  30

#if defined(RESTART_TERMINATED_SERVICES) || defined(RESTART_FAILED_SERVICES)
static time_t compute_backoff(int restart_count) {
  time_t delay = BACKOFF_BASE_SECS;
  for (int i = 0; i < restart_count && delay < BACKOFF_MAX_SECS; i++)
    delay *= 2;
  return delay < BACKOFF_MAX_SECS ? delay : BACKOFF_MAX_SECS;
}
#endif

static void do_restart(Service *service) {
  char *name_dup = strdup(service->args[0]);
  char *name = basename(name_dup);
  LOG_WARN("Restarting service (%s)...", name);
  pid_t new_pid = fork();
  if (new_pid == 0) {
    setpgid(0, 0);
    expand_variables(service->args);
    handle_redirection(service->args);
    execvp(service->args[0], service->args);
    LOG_ERROR("Service (%s) failed to be restarted: %s", name, strerror(errno));
    exit(EXIT_FAILURE);
  } else {
    service->pid = new_pid;
    service->restart_count++;
    service->next_restart = 0;
  }
  free(name_dup);
}

void handle_child_exit(pid_t pid, int status) {
  if (pid > 0) {
    Service *service = find_service(pid);
    if (service != NULL) {
      char *service_name_dup = strdup(service->args[0]);

#if defined(RESTART_TERMINATED_SERVICES)
      {
        char *service_name = basename(service_name_dup);
        if (service->restart_count < MAX_RESTARTS) {
          time_t delay = compute_backoff(service->restart_count);
          service->next_restart = time(NULL) + delay;
          LOG_WARN("Service (%s) terminated. Restarting in %lds...",
                   service_name, (long)delay);
        } else {
          LOG_ERROR("Service (%s) has reached the maximum restart limit.",
                    service_name);
          remove_service(pid);
        }
      }
#elif defined(RESTART_FAILED_SERVICES)
      {
        char *service_name = basename(service_name_dup);
        if ((WIFEXITED(status) && WEXITSTATUS(status) != 0) ||
            WIFSIGNALED(status)) {
          if (service->restart_count < MAX_RESTARTS) {
            time_t delay = compute_backoff(service->restart_count);
            service->next_restart = time(NULL) + delay;
            if (WIFEXITED(status)) {
              LOG_WARN("Service (%s) failed with status %d. Restarting in %lds...",
                       service_name, WEXITSTATUS(status), (long)delay);
            } else {
              LOG_WARN("Service (%s) terminated by signal %d. Restarting in %lds...",
                       service_name, WTERMSIG(status), (long)delay);
            }
          } else {
            LOG_ERROR("Service (%s) has reached the maximum restart limit.",
                      service_name);
            remove_service(pid);
          }
        } else {
          remove_service(pid);
        }
      }
#else
      remove_service(pid);
#endif
      free(service_name_dup);
    }
  }

#ifdef REINITIALIZE_ON_ALL_SERVICE_TERMINATION
  if (get_service_count() == 0) {
    LOG_FATAL("All services terminated. Reinitializing...");
    execvp(usagi_argv[0], usagi_argv);
    LOG_ERROR("Failed to restart UsagiInit: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }
#endif
}

void restart_pending_services(void) {
  Service *service;
  while ((service = get_service_pending_restart(time(NULL))) != NULL)
    do_restart(service);
}
