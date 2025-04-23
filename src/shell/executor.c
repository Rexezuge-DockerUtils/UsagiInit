#include "shell/executor.h"
#include "shell/parser.h"
#include "shell/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ARGS 128

static void execute_pipeline(char *cmdline) {
  char *cmds[10];
  int num_cmds = 0;

  cmds[num_cmds++] = strtok(cmdline, "|");
  while ((cmds[num_cmds++] = strtok(NULL, "|")) != NULL)
    ;

  int fd[2], in_fd = 0;
  for (int i = 0; i < num_cmds - 1; ++i) {
    pipe(fd);
    if (fork() == 0) {
      setpgid(0, 0); // New process group
      dup2(in_fd, STDIN_FILENO);
      if (i < num_cmds - 2)
        dup2(fd[1], STDOUT_FILENO);
      close(fd[0]);

      char *args[MAX_ARGS];
      parse_command(cmds[i], args);
      handle_redirection(args);
      execvp(args[0], args);
      perror("exec");
      exit(EXIT_FAILURE);
    }
    wait(NULL);
    close(fd[1]);
    in_fd = fd[0];
  }
}

void run_command(char *line) {
  int background = 0;
  if (strchr(line, '|') != NULL) {
    execute_pipeline(line);
    return;
  }

  if (strchr(line, '&')) {
    background = 1;
    *strchr(line, '&') = '\0';
  }

  char *args[MAX_ARGS];
  parse_command(line, args);
  if (args[0] == NULL)
    return;

  if (strcmp(args[0], "cd") == 0) {
    if (args[1] == NULL || chdir(args[1]) != 0)
      perror("cd");
    return;
  }

  if (strcmp(args[0], "exit") == 0)
    exit(0);

  pid_t pid = fork();
  if (pid == 0) {
    setpgid(0, 0); // Make the child the leader of a new process group
    handle_redirection(args);
    execvp(args[0], args);
    perror("execvp");
    exit(EXIT_FAILURE);
  } else if (!background) {
    setpgid(pid, pid); // Ensure process group is set in parent context as well
    waitpid(pid, NULL, 0);
  }
}
