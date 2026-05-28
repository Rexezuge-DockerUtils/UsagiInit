#include "shell/utils.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void remove_args(char **args, int index, int count) {
  int j = index;
  while (args[j + count] != NULL) {
    args[j] = args[j + count];
    j++;
  }
  args[j] = NULL;
}

static void redirect_to_file(int target_fd, const char *path,
                             const char *error_context, int flags) {
  if (path == NULL || path[0] == '\0') {
    fprintf(stderr, "%s: missing file operand\n", error_context);
    exit(EXIT_FAILURE);
  }

  int fd = open(path, flags, 0644);
  if (fd < 0) {
    perror(error_context);
    exit(EXIT_FAILURE);
  }
  if (dup2(fd, target_fd) < 0) {
    perror("dup2");
    close(fd);
    exit(EXIT_FAILURE);
  }
  close(fd);
}

void handle_redirection(char **args) {
  for (int i = 0; args[i] != NULL; ++i) {
    if (strcmp(args[i], "<") == 0) {
      redirect_to_file(STDIN_FILENO, args[i + 1], "open for input", O_RDONLY);
      remove_args(args, i, 2);
      i--; // Recheck current position
    } else if (strcmp(args[i], ">") == 0) {
      redirect_to_file(STDOUT_FILENO, args[i + 1], "open for output",
                       O_CREAT | O_WRONLY | O_TRUNC);
      remove_args(args, i, 2);
      i--; // Recheck current position
    } else if (strcmp(args[i], "2>") == 0) {
      redirect_to_file(STDERR_FILENO, args[i + 1], "open for stderr",
                       O_CREAT | O_WRONLY | O_TRUNC);
      remove_args(args, i, 2);
      i--; // Recheck current position
    } else if (strncmp(args[i], "2>", 2) == 0) {
      const char *target = args[i] + 2;
      if (strcmp(target, "&1") == 0) {
        if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0) {
          perror("dup2 stderr to stdout");
          exit(EXIT_FAILURE);
        }
      } else {
        redirect_to_file(STDERR_FILENO, target, "open for stderr",
                         O_CREAT | O_WRONLY | O_TRUNC);
      }
      remove_args(args, i, 1);
      i--; // Recheck current position
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
