#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "usagi-reg: missing command\n");
    return 1;
  }

  const char *fd_str = getenv("USAGI_SVC_FD");
  if (!fd_str) {
    fprintf(stderr, "usagi-reg: USAGI_SVC_FD not set\n");
    return 1;
  }
  int reg_fd = atoi(fd_str);

  /* A sync pipe ensures the service child cannot exec until AFTER the parent
   * has written the registration message.  This guarantees that UsagiInit
   * can log "SERVICE: ..." before the service produces any output. */
  int sync[2];
  if (pipe(sync) < 0) {
    perror("usagi-reg: pipe");
    return 1;
  }

  pid_t pid = fork();
  if (pid < 0) {
    perror("usagi-reg: fork");
    return 1;
  }

  if (pid == 0) {
    /* Child: wait for parent to signal (close write end) then exec. */
    close(reg_fd);
    close(sync[1]);
    char dummy;
    read(sync[0], &dummy, 1);
    close(sync[0]);
    execvp(argv[1], &argv[1]);
    perror(argv[1]);
    _exit(127);
  }

  /* Parent: write registration, then signal the child. */
  close(sync[0]);

  /* Build registration message: pid\nargc\narg0\narg1\n...argN\n
   * Written in a single write() for pipe atomicity. */
  char buf[4096];
  int  len    = 0;
  int  n_args = argc - 1;

  len += snprintf(buf + len, sizeof(buf) - len, "%d\n%d\n", (int)pid, n_args);
  for (int i = 1; i < argc && len < (int)sizeof(buf) - 1; i++)
    len += snprintf(buf + len, sizeof(buf) - len, "%s\n", argv[i]);

  ssize_t written = 0;
  while (written < len) {
    ssize_t r = write(reg_fd, buf + written, len - written);
    if (r < 0) {
      perror("usagi-reg: write");
      break;
    }
    written += r;
  }

  close(reg_fd);
  close(sync[1]); /* Signal child to exec. */
  return 0;
}
