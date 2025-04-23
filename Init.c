#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 128

// 解析命令，分割成参数数组
void parse_command(char *line, char **args) {
  int i = 0;
  char *token = strtok(line, " \t\n");
  while (token != NULL && i < MAX_ARGS - 1) {
    args[i++] = token;
    token = strtok(NULL, " \t\n");
  }
  args[i] = NULL;
}

// 处理重定向
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

// 处理管道
void execute_pipeline(char *cmdline) {
  char *cmds[10];
  int num_cmds = 0;

  cmds[num_cmds++] = strtok(cmdline, "|");
  while ((cmds[num_cmds++] = strtok(NULL, "|")) != NULL)
    ;

  int fd[2], in_fd = 0;
  for (int i = 0; i < num_cmds - 1; ++i) {
    pipe(fd);
    if (fork() == 0) {
      dup2(in_fd, 0);
      if (i < num_cmds - 2)
        dup2(fd[1], 1);
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

// 运行一条命令
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

  // 内建命令
  if (strcmp(args[0], "cd") == 0) {
    if (args[1] == NULL || chdir(args[1]) != 0) {
      perror("cd");
    }
    return;
  }
  if (strcmp(args[0], "exit") == 0) {
    exit(0);
  }

  // 外部命令执行
  pid_t pid = fork();
  if (pid == 0) {
    handle_redirection(args);
    execvp(args[0], args);
    perror("execvp");
    exit(EXIT_FAILURE);
  } else if (!background) {
    waitpid(pid, NULL, 0);
  }
}

// 主循环
int main() {
  char line[MAX_CMD_LEN];

  while (1) {
    printf("$> ");
    fflush(stdout);

    if (!fgets(line, sizeof(line), stdin))
      break;

    run_command(line);
  }

  return 0;
}
