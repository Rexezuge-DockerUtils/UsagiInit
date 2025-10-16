#include "guardian.h"
#include "globals.h"
#include "logger.h"
#include "services.h"

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef RELEASE_MODE
#define MAX_RESTARTS 999
#else
#define MAX_RESTARTS 2
#endif

void handle_child_exit(pid_t pid, int status) {
  if (pid > 0) {
    Service *service = find_service(pid);
    if (service != NULL) {
      char *service_name_dup = strdup(service->args[0]);
      char *service_name = basename(service_name_dup);

#if defined(RESTART_TERMINATED_SERVICES)
      // Restart any terminated service
      if (service->restart_count < MAX_RESTARTS) {
        LOG_WARN("Service (%s) terminated. Restarting...", service_name);
        pid_t new_pid = fork();
        if (new_pid == 0) {
          execvp(service->args[0], service->args);
          LOG_ERROR("Service (%s) failed to be restarted: %s", service_name,
                    strerror(errno));
          exit(EXIT_FAILURE);
        } else {
          service->pid = new_pid;
          service->restart_count++;
        }
      } else {
        LOG_ERROR("Service (%s) has reached the maximum restart limit.",
                  service_name);
        remove_service(pid);
      }
#elif defined(RESTART_FAILED_SERVICES)
      // Existing logic: Restart only failed services (non-zero exit or signal)
      if ((WIFEXITED(status) && WEXITSTATUS(status) != 0) ||
          WIFSIGNALED(status)) {
        if (service->restart_count < MAX_RESTARTS) {
          if (WIFEXITED(status)) {
            LOG_WARN("Service (%s) failed with status %d. Restarting...",
                     service_name, WEXITSTATUS(status));
          } else {
            LOG_WARN("Service (%s) terminated by signal %d. Restarting...",
                     service_name, WTERMSIG(status));
          }
          pid_t new_pid = fork();
          if (new_pid == 0) {
            execvp(service->args[0], service->args);
            LOG_ERROR("Service (%s) failed to be restarted: %s", service_name,
                      strerror(errno));
            exit(EXIT_FAILURE);
          } else {
            service->pid = new_pid;
            service->restart_count++;
          }
        } else {
          LOG_ERROR("Service (%s) has reached the maximum restart limit.",
                    service_name);
          remove_service(pid);
        }
      } else {
        // If RESTART_FAILED_SERVICES is enabled, but the service didn't fail,
        // just remove it.
        remove_service(pid);
      }
#else
      // Both flags disabled: remove service immediately
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
