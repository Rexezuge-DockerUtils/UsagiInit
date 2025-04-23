#include "shell/command.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

pid_t run_executable(const command_t *cmd) {
  if (!cmd || !cmd->executable || !cmd->args) {
    fprintf(stderr, "Invalid command structure\n");
    return -1;
  }

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork failed");
    return -1;
  }

  if (pid == 0) {
    // Child process

    // Input redirection
    if (cmd->stdin_path) {
      int fd_in = open(cmd->stdin_path, O_RDONLY);
      if (fd_in < 0) {
        perror("Failed to open stdin file");
        exit(EXIT_FAILURE);
      }
      dup2(fd_in, STDIN_FILENO);
      close(fd_in);
    }

    // Output redirection
    if (cmd->stdout_path) {
      int flags =
          O_WRONLY | O_CREAT | (cmd->append_stdout ? O_APPEND : O_TRUNC);
      int fd_out = open(cmd->stdout_path, flags, 0644);
      if (fd_out < 0) {
        perror("Failed to open stdout file");
        exit(EXIT_FAILURE);
      }
      dup2(fd_out, STDOUT_FILENO);
      close(fd_out);
    }

    // Error redirection
    if (cmd->stderr_path) {
      int flags =
          O_WRONLY | O_CREAT | (cmd->append_stderr ? O_APPEND : O_TRUNC);
      int fd_err = open(cmd->stderr_path, flags, 0644);
      if (fd_err < 0) {
        perror("Failed to open stderr file");
        exit(EXIT_FAILURE);
      }
      dup2(fd_err, STDERR_FILENO);
      close(fd_err);
    }

    // Execute the program
    execvp(cmd->executable, cmd->args);

    // If execvp fails
    perror("execvp failed");
    exit(EXIT_FAILURE);
  }

  // Parent process returns the child PID
  return pid;
}

int command_sample_main() {
  char *args[] = {"./a.out", "-g", "-c", "./config.json", NULL};

  command_t cmd = {.executable = "./a.out",
                   .args = args,
                   .stdin_path = "input.txt",
                   .stdout_path = "/dev/null",
                   .stderr_path = "/dev/null",
                   .append_stdout = 0,
                   .append_stderr = 0};

  pid_t child_pid = run_executable(&cmd);
  if (child_pid > 0) {
    printf("Started child process with PID %d\n", child_pid);
  }

  return 0;
}
