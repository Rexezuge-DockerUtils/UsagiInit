#include "gVariables.h"
#include "logger.h"
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

  const char *script_path = "UsagiInit.sh";
  int script_fd = -1;

  if (argc > 1 && argv[1] != NULL) {
    script_path = argv[1];
    LOG_INFO("Using script path from argument: %s", script_path);
  } else {
    LOG_INFO("No script path provided, using default: %s", script_path);
  }

  // Attempt to open script file
  LOG_INFO("Attempting to open script file: %s", script_path);
  script_fd = open(script_path, O_RDONLY);
  if (script_fd < 0) {
    LOG_WARN("Script file not found or cannot be opened (%s). Falling back to "
             "terminal input.",
             strerror(errno));
  } else {
    LOG_INFO("Script file opened successfully: fd=%d", script_fd);

    // Redirect stdin to the file
    LOG_INFO("Redirecting stdin to script file");
    if (dup2(script_fd, STDIN_FILENO) < 0) {
      LOG_ERROR("Failed to redirect stdin: %s", strerror(errno));
      close(script_fd);
      return EXIT_FAILURE;
    }
    close(script_fd);
    interactivity = NONINTERACTIVE;
    LOG_INFO("Stdin successfully redirected");
  }

  char *line = (char *)malloc(MAX_CMD_LEN);
  if (!line) {
    LOG_ERROR("malloc: %s", strerror(errno));
    return EXIT_FAILURE;
  }

  setup_signal_forwarding();

  phase = PHASE_SHELL;

  while (1) {
    prompt_for_intput();

    if (!fgets(line, MAX_CMD_LEN, stdin))
      break;

    run_command(line);
  }

  free(line);
  LOG_INFO("Init Complete");
  phase = PHASE_GUARDIAN;
  fflush(stdout);

  while (1) {
    waitpid(-1, NULL, 0);
  }

  return 0;
}
