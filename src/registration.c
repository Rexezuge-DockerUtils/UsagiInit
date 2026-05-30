#include "registration.h"
#include "internal.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_CAP (64 * 1024)

/* Non-reentrant: single static buffer; only one registration fd may be active.
 */
static char s_buf[BUF_CAP];
static int s_start = 0;
static int s_end = 0;

static int buf_fill(int fd) {
  if (s_start > 0) {
    memmove(s_buf, s_buf + s_start, s_end - s_start);
    s_end -= s_start;
    s_start = 0;
  }
  ssize_t n = read(fd, s_buf + s_end, BUF_CAP - s_end - 1);
  if (n > 0) {
    s_end += n;
    return 1;
  }
  if (n == 0)
    return REG_EOF;
  if (errno == EAGAIN || errno == EWOULDBLOCK)
    return REG_AGAIN;
  return REG_ERROR;
}

static int buf_read_line(char *out, int cap) {
  for (int i = s_start; i < s_end; i++) {
    if (s_buf[i] == '\n') {
      int len = i - s_start;
      if (len >= cap)
        len = cap - 1;
      memcpy(out, s_buf + s_start, len);
      out[len] = '\0';
      s_start = i + 1;
      return 0;
    }
  }
  return -1;
}

int read_registration(int fd, pid_t *pid_out, char ***argv_out,
                      char ***env_out) {
  int fill = buf_fill(fd);
  if (fill == REG_EOF && s_start == s_end)
    return REG_EOF;
  if (fill == REG_AGAIN && s_start == s_end)
    return REG_AGAIN;

  char tmp[4096];
  int save_start = s_start;

  if (buf_read_line(tmp, sizeof(tmp)) < 0) {
    s_start = save_start;
    return REG_AGAIN;
  }
  pid_t pid = (pid_t)atoi(tmp);

  if (buf_read_line(tmp, sizeof(tmp)) < 0) {
    s_start = save_start;
    return REG_AGAIN;
  }
  int argc = atoi(tmp);
  if (argc <= 0) {
    s_start = save_start;
    return REG_ERROR;
  }

  char **argv = calloc(argc + 1, sizeof(char *));
  if (!argv)
    return REG_ERROR;

  for (int i = 0; i < argc; i++) {
    if (buf_read_line(tmp, sizeof(tmp)) < 0) {
      for (int j = 0; j < i; j++)
        free(argv[j]);
      free(argv);
      s_start = save_start;
      return REG_AGAIN;
    }
    argv[i] = strdup(tmp);
    if (!argv[i]) {
      for (int j = 0; j < i; j++)
        free(argv[j]);
      free(argv);
      s_start = save_start;
      return REG_ERROR;
    }
  }
  argv[argc] = NULL;

  /* Parse environment: envc\nenv0\n...envN\n */
  if (buf_read_line(tmp, sizeof(tmp)) < 0) {
    for (int j = 0; j < argc; j++)
      free(argv[j]);
    free(argv);
    s_start = save_start;
    return REG_AGAIN;
  }
  int envc = atoi(tmp);
  if (envc < 0) {
    for (int j = 0; j < argc; j++)
      free(argv[j]);
    free(argv);
    s_start = save_start;
    return REG_ERROR;
  }

  char **env = calloc(envc + 1, sizeof(char *));
  if (!env) {
    for (int j = 0; j < argc; j++)
      free(argv[j]);
    free(argv);
    return REG_ERROR;
  }

  for (int i = 0; i < envc; i++) {
    if (buf_read_line(tmp, sizeof(tmp)) < 0) {
      for (int j = 0; j < i; j++)
        free(env[j]);
      free(env);
      for (int j = 0; j < argc; j++)
        free(argv[j]);
      free(argv);
      s_start = save_start;
      return REG_AGAIN;
    }
    env[i] = strdup(tmp);
    if (!env[i]) {
      for (int j = 0; j < i; j++)
        free(env[j]);
      free(env);
      for (int j = 0; j < argc; j++)
        free(argv[j]);
      free(argv);
      s_start = save_start;
      return REG_ERROR;
    }
  }
  env[envc] = NULL;

  *pid_out = pid;
  *argv_out = argv;
  *env_out = env;
  return REG_OK;
}

void free_registration_argv(char **argv) {
  if (!argv)
    return;
  for (int i = 0; argv[i]; i++)
    free(argv[i]);
  free(argv);
}

void free_registration_env(char **env) {
  if (!env)
    return;
  for (int i = 0; env[i]; i++)
    free(env[i]);
  free(env);
}

#ifdef UNIT_TESTING
void registration_reset(void) {
  s_start = 0;
  s_end = 0;
}
#endif
