#include "shell/utils.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void handle_redirection(char **args) {
  for (int i = 0; args[i] != NULL; ++i) {
    if (strcmp(args[i], "<") == 0) {
      int fd = open(args[i + 1], O_RDONLY);
      if (fd < 0) {
        perror("open for input");
        exit(EXIT_FAILURE);
      }
      dup2(fd, STDIN_FILENO);
      close(fd);
      args[i] = NULL;
    } else if (strcmp(args[i], ">") == 0) {
      int fd = open(args[i + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
      if (fd < 0) {
        perror("open for output");
        exit(EXIT_FAILURE);
      }
      dup2(fd, STDOUT_FILENO);
      close(fd);
      args[i] = NULL;
    }
  }
}

char *trim_whitespace(char *str) {
  while (*str == ' ' || *str == '\t')
    str++;
  if (*str == 0)
    return str;
  char *end = str + strlen(str) - 1;
  while (end > str && (*end == ' ' || *end == '\t' || *end == '\n'))
    end--;
  *(end + 1) = 0;
  return str;
}

void expand_variables(char **args) {
  for (int i = 0; args[i] != NULL; i++) {
    if (args[i][0] == '$') {
      char *var_name = args[i] + 1; // Skip the '$'
      char *value = getenv(var_name);
      if (value != NULL) {
        args[i] = value;
      }
    }
  }
}
