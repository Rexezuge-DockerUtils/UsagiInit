#include "guardian.h"
#include "logger.h"
#include "services.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef RELEASE_MODE
#define MAX_RESTARTS 10
#else
#define MAX_RESTARTS 2
#endif

void handle_child_exit(pid_t pid, int status) {
  if (pid > 0) {
    Service *service = find_service(pid);
    if (service != NULL) {
      if ((WIFEXITED(status) && WEXITSTATUS(status) != 0) ||
          WIFSIGNALED(status)) {
        if (service->restart_count < MAX_RESTARTS) {
          if (WIFEXITED(status)) {
            LOG_WARN("Service (PID: %d) failed with status %d. Restarting...",
                     pid, WEXITSTATUS(status));
          } else {
            LOG_WARN("Service (PID: %d) terminated by signal %d. Restarting...",
                     pid, WTERMSIG(status));
          }
          pid_t new_pid = fork();
          if (new_pid == 0) {
            execvp(service->args[0], service->args);
            LOG_ERROR("Failed to restart service: %s", strerror(errno));
            exit(EXIT_FAILURE);
          } else {
            service->pid = new_pid;
            service->restart_count++;
          }
        } else {
          LOG_ERROR("Service (PID: %d) has reached the maximum restart limit.",
                    pid);
          remove_service(pid);
        }
      } else {
        remove_service(pid);
      }
    }
  }
}
