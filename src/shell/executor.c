#include "shell/executor.h"
#include "logger.h"
#include "services.h"
#include "shell/parser.h"
#include "shell/utils.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ARGS 128
#define MAX_CMDS 10

static void execute_pipeline(char *cmdline) {
  char *cmds[MAX_CMDS];
  int num_cmds = 0;

  // Tokenize pipeline
  cmds[num_cmds++] = trim_whitespace(strtok(cmdline, "|"));
  while (num_cmds < MAX_CMDS &&
         (cmds[num_cmds++] = trim_whitespace(strtok(NULL, "|"))) != NULL)
    ;

  int fd[2], in_fd = 0;
  for (int i = 0; i < num_cmds - 1; ++i) {
    if (pipe(fd) < 0) {
      LOG_ERROR("pipe() error");
      exit(EXIT_FAILURE);
    }
    if (fork() == 0) {
      setpgid(0, 0); // New process group
      dup2(in_fd, STDIN_FILENO);
      if (i < num_cmds - 2) {
        dup2(fd[1], STDOUT_FILENO);
      }
      close(fd[0]);
      close(fd[1]);

      char *args[MAX_ARGS];
      parse_command(cmds[i], args);
      expand_variables(args);
      handle_redirection(args);

      execvp(args[0], args);
      LOG_ERROR("%s: %s\n", args[0], strerror(errno));
      exit(EXIT_FAILURE);
    }

    wait(NULL);
    close(fd[1]);
    in_fd = fd[0];
  }

  // Final command in pipeline
  if (fork() == 0) {
    setpgid(0, 0);
    dup2(in_fd, STDIN_FILENO);

    char *args[MAX_ARGS];
    parse_command(cmds[num_cmds - 1], args);
    expand_variables(args);
    handle_redirection(args);

    execvp(args[0], args);
    LOG_ERROR("%s: %s\n", args[0], strerror(errno));
    exit(EXIT_FAILURE);
  }

  close(in_fd);
  wait(NULL);
}

void run_command(char *line) {
  line = trim_whitespace(line);

  // Skip empty lines and comments (lines starting with #)
  if (line[0] == '\0' || line[0] == '#' || line[0] == '\n') {
    return;
  }

  int background = 0;
  if (strchr(line, '&')) {
    background = 1;
    *strchr(line, '&') = '\0';
  }

  if (background) {
    LOG_INFO("SERVICE: %s", line);
  } else {
    LOG_INFO("RUN: %s", line);
  }

  if (strchr(line, '|') != NULL) {
    execute_pipeline(line);
    return;
  }

  char *args[MAX_ARGS];
  parse_command(line, args);
  expand_variables(args);

  if (args[0] == NULL)
    return;

  if (strcmp(args[0], "export") == 0) {
    if (args[1] != NULL) {
      char *name = strtok(args[1], "=");
      char *value = strtok(NULL, "");
      if (name != NULL && value != NULL) {
        setenv(name, value, 1);
      } else {
        LOG_ERROR("export: invalid format. Use VAR=value\n");
      }
    }
    return;
  }

  if (strcmp(args[0], "cd") == 0) {
    if (args[1] == NULL || chdir(args[1]) != 0)
      LOG_ERROR("cd: %s\n", strerror(errno));
    return;
  }

  if (strcmp(args[0], "exit") == 0) {
    exit(0);
  }

  pid_t pid = fork();
  if (pid == 0) {
    setpgid(0, 0);
    handle_redirection(args);
    execvp(args[0], args);
    LOG_ERROR("%s: %s\n", args[0], strerror(errno));
    exit(EXIT_FAILURE);
  } else if (background) {
    add_service(pid, args);
  } else {
    setpgid(pid, pid);
    waitpid(pid, NULL, 0);
  }
}
