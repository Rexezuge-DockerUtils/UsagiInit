#include "shell/executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_CMD_LEN 1024

int main(void) {
  char *line = (char *)malloc(MAX_CMD_LEN);
  if (!line) {
    perror("malloc");
    return EXIT_FAILURE;
  }

  while (1) {
    printf("$> ");
    fflush(stdout);

    if (!fgets(line, MAX_CMD_LEN, stdin))
      break;

    run_command(line);
  }

  free(line);
  printf("\nInit Complete\n");
  fflush(stdout);

  while (1)
    waitpid(-1, NULL, 0);

  return 0;
}
