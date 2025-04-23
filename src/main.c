#include "logger.h"
#include "shell/executor.h"
#include "signals.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_CMD_LEN 1024

int main(void) {
  char *line = (char *)malloc(MAX_CMD_LEN);
  if (!line) {
    LOG_ERROR("fopen: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  setup_signal_forwarding();

  while (1) {
    printf("$> ");
    fflush(stdout);

    if (!fgets(line, MAX_CMD_LEN, stdin))
      break;

    run_command(line);
  }

  free(line);
  LOG_INFO("Init Complete");
  fflush(stdout);

  while (1) {
    waitpid(-1, NULL, 0);
  }

  return 0;
}
