#include "gVariables.h"
#include "logger.h"
#include "services.h"
#include "shell/executor.h"
#include "shell/prompt.h"
#include "signals.h"
#include "types.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_CMD_LEN 1024

int main(int argc, char *argv[]) {
  LOG_INFO("          /\\_/\\");
  LOG_INFO("         ( o.o )        UsagiInit");
  LOG_INFO("          > ^ <");

  LOG_INFO("Init Begin");

  const char *script_path = "./UsagiInit.sh";
  int script_fd = -1;

  if (argc > 1 && argv[1] != NULL) {
    script_path = argv[1];
  } else {
    LOG_WARN("No Script Path Provided, Using Default: %s", script_path);
  }

  // Attempt to open script file
  LOG_INFO("Attempting to Open Script: %s", script_path);
  script_fd = open(script_path, O_RDONLY);
  if (script_fd < 0) {
    LOG_WARN("Script Failed to be Opened (%s). Falling Back to "
             "Terminal Input",
             strerror(errno));
  } else {
    LOG_DEBUG("Script Opened Successfully: fd=%d", script_fd);
    LOG_DEBUG("Redirecting stdin to Script File");
    if (dup2(script_fd, STDIN_FILENO) < 0) {
      LOG_ERROR("Failed to Redirect stdin: %s", strerror(errno));
      close(script_fd);
      exit(EXIT_FAILURE);
    }
    close(script_fd);
    interactivity = NONINTERACTIVE;
    LOG_DEBUG("stdin Successfully Redirected");
  }

  char *line = (char *)malloc(MAX_CMD_LEN);
  if (!line) {
    LOG_ERROR("malloc: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  setup_signal_forwarding();

  phase = PHASE_SHELL;

  while (1) {
    prompt_for_intput();

    if (!fgets(line, MAX_CMD_LEN, stdin)) {
      break;
    }

    run_command(line);
  }

  free(line);
  LOG_INFO("Init Complete");
  phase = PHASE_GUARDIAN;
  fflush(stdout);

  #ifdef RELEASE_MODE
#define MAX_RESTARTS 10
#else
#define MAX_RESTARTS 2
#endif

  while (1) {
    int status;
    pid_t pid = waitpid(-1, &status, 0);

    if (pid > 0) {
      Service *service = find_service(pid);
      if (service != NULL) {
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
          if (service->restart_count < MAX_RESTARTS) {
            LOG_WARN("Service (PID: %d) failed with status %d. Restarting...",
                     pid, WEXITSTATUS(status));
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
            LOG_ERROR(
                "Service (PID: %d) has reached the maximum restart limit.",
                pid);
            remove_service(pid);
          }
        } else {
          remove_service(pid);
        }
      }
    }
  }

  exit(EXIT_SUCCESS);
}
