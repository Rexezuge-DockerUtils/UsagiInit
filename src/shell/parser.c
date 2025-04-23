#include "shell/parser.h"
#include <string.h>

void parse_command(char *line, char **args) {
  int i = 0;
  char *token = strtok(line, " \t\n");
  while (token != NULL && i < MAX_ARGS - 1) {
    args[i++] = token;
    token = strtok(NULL, " \t\n");
  }
  args[i] = NULL;
}
