#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char **environ;

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

  int n_args = argc - 1;
  int envc   = 0;
  while (environ[envc]) envc++;

  /* Calculate total message size: pid\nargc\narg0\n...argN\nenvc\nenv0\n...envM\n */
  size_t size = 0;
  {
    char tmp[32];
    size += (size_t)snprintf(tmp, sizeof(tmp), "%d\n%d\n", (int)pid, n_args);
  }
  for (int i = 1; i < argc; i++)
    size += strlen(argv[i]) + 1;
  {
    char tmp[32];
    size += (size_t)snprintf(tmp, sizeof(tmp), "%d\n", envc);
  }
  for (int i = 0; i < envc; i++)
    size += strlen(environ[i]) + 1;

  char *buf = malloc(size + 1);
  if (!buf) {
    perror("usagi-reg: malloc");
    close(reg_fd);
    close(sync[1]);
    return 1;
  }

  int pos = 0;
  pos += sprintf(buf + pos, "%d\n%d\n", (int)pid, n_args);
  for (int i = 1; i < argc; i++)
    pos += sprintf(buf + pos, "%s\n", argv[i]);
  pos += sprintf(buf + pos, "%d\n", envc);
  for (int i = 0; i < envc; i++)
    pos += sprintf(buf + pos, "%s\n", environ[i]);

  ssize_t written = 0;
  while (written < (ssize_t)pos) {
    ssize_t r = write(reg_fd, buf + written, (size_t)(pos - written));
    if (r < 0) {
      perror("usagi-reg: write");
      break;
    }
    written += r;
  }

  free(buf);
  close(reg_fd);
  close(sync[1]); /* Signal child to exec. */
  return 0;
}
