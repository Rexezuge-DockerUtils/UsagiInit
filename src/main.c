#include "globals.h"
#include "guardian.h"
#include "logger.h"
#include "registration.h"
#include "services.h"
#include "shell/executor.h"
#include "shell/preprocess.h"
#include "shell/prompt.h"
#include "signals.h"
#include "types.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_CMD_LEN 1024

/* Prepend the directory containing this executable to PATH so that
 * usagi-reg (compiled alongside UsagiInit) is always findable by sh. */
static void prepend_self_to_path(void) {
  char self[PATH_MAX];
  ssize_t n = readlink("/proc/self/exe", self, sizeof(self) - 1);
  if (n <= 0) return;
  self[n] = '\0';

  char *slash = strrchr(self, '/');
  if (!slash) return;
  *slash = '\0';

  const char *existing = getenv("PATH");
  char       *new_path;
  if (existing && existing[0]) {
    if (asprintf(&new_path, "%s:%s", self, existing) < 0) return;
  } else {
    new_path = strdup(self);
  }
  if (new_path) {
    setenv("PATH", new_path, 1);
    free(new_path);
  }
}

/* Run the guardian loop.  Called after all shell-phase work is done. */
static void run_guardian(void) {
  /* Drain any zombies that accumulated during the shell phase. */
  {
    int     status;
    pid_t   pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
      handle_child_exit(pid, status);
  }

  /* Nothing to guard — exit cleanly (e.g. script called 'exit' with no services). */
  if (get_service_count() == 0)
    exit(EXIT_SUCCESS);

  while (1) {
    time_t deadline = get_next_restart_time();
    int    status;
    pid_t  pid;
    if (deadline > 0) {
      pid = waitpid(-1, &status, WNOHANG);
      if (pid == 0) {
        struct timespec ts = {0, 50 * 1000000L};
        nanosleep(&ts, NULL);
      }
    } else {
      pid = waitpid(-1, &status, 0);
    }
    if (pid > 0) handle_child_exit(pid, status);
    restart_pending_services();
  }
}

int main(int argc, char *argv[]) {
  usagi_argv = argv;

  LOG_INFO("          /\\_/\\");
  LOG_INFO("         ( o.o )        UsagiInit");
  LOG_INFO("          > ^ <");
  LOG_INFO("Init Begin");

  const char *script_path = "./UsagiInit.sh";
  if (argc > 1 && argv[1] != NULL) {
    script_path = argv[1];
  } else {
    LOG_DEBUG("No Script Path Provided, Using Default: %s", script_path);
  }

  LOG_INFO("Attempting to Open Script: %s", script_path);
  int script_fd = open(script_path, O_RDONLY);
  if (script_fd < 0) {
    LOG_WARN("Script Failed to be Opened (%s). Falling Back to "
             "Terminal Input",
             strerror(errno));
  } else {
    LOG_DEBUG("Script Opened Successfully: fd=%d", script_fd);
    close(script_fd);
  }

  /* Become subreaper: orphaned service children are reparented to us rather
   * than to init, ensuring we receive their SIGCHLD outside a container too. */
  prctl(PR_SET_CHILD_SUBREAPER, 1);

  setup_signal_forwarding();
  phase = PHASE_SHELL;

  if (script_fd >= 0) {
    /* ── Script mode: preprocess then run via /bin/sh ── */
    char *processed = preprocess_script(script_path);
    if (!processed) {
      LOG_ERROR("Failed to preprocess script: %s", strerror(errno));
      exit(EXIT_FAILURE);
    }

    int reg_pipe[2];
    if (pipe(reg_pipe) < 0) {
      LOG_ERROR("pipe: %s", strerror(errno));
      free(processed);
      exit(EXIT_FAILURE);
    }

    char fd_str[16];
    snprintf(fd_str, sizeof(fd_str), "%d", reg_pipe[1]);
    setenv("USAGI_SVC_FD", fd_str, 1);

    prepend_self_to_path();

    LOG_INFO("Shell Begin");

    pid_t sh_pid = fork();
    if (sh_pid < 0) {
      LOG_ERROR("fork: %s", strerror(errno));
      close(reg_pipe[0]);
      close(reg_pipe[1]);
      unlink(processed);
      free(processed);
      exit(EXIT_FAILURE);
    }
    if (sh_pid == 0) {
      close(reg_pipe[0]);
      execl("/bin/sh", "sh", processed, NULL);
      perror("execl /bin/sh");
      _exit(EXIT_FAILURE);
    }

    /* Parent: close write end, read registrations while sh runs. */
    close(reg_pipe[1]);
    fcntl(reg_pipe[0], F_SETFL, O_NONBLOCK);

    int sh_done = 0;
    while (!sh_done) {
      /* Wait up to 5ms for a registration to arrive on the pipe, then
       * check whether sh has exited.  poll() lets us react immediately
       * when usagi-reg writes a registration (no fixed polling delay),
       * which keeps the SERVICE: log ahead of the service's own output. */
      struct pollfd pfd = {reg_pipe[0], POLLIN, 0};
      poll(&pfd, 1, 5);

      pid_t   svc_pid;
      char  **svc_args;
      int     r;
      while ((r = read_registration(reg_pipe[0], &svc_pid, &svc_args)) == REG_OK) {
        LOG_INFO("SERVICE: %s", svc_args[0]);
        add_service(svc_pid, svc_args);
        free_registration_argv(svc_args);
      }

      int    sh_status;
      pid_t  exited = waitpid(sh_pid, &sh_status, WNOHANG);
      if (exited == sh_pid || (exited < 0 && errno != EINTR))
        sh_done = 1;
    }

    /* Drain any registrations buffered after sh exited. */
    fcntl(reg_pipe[0], F_SETFL, 0);
    {
      pid_t  svc_pid;
      char **svc_args;
      int    r;
      while ((r = read_registration(reg_pipe[0], &svc_pid, &svc_args)) == REG_OK) {
        LOG_INFO("SERVICE: %s", svc_args[0]);
        add_service(svc_pid, svc_args);
        free_registration_argv(svc_args);
      }
    }

    close(reg_pipe[0]);
    unlink(processed);
    free(processed);

  } else {
    /* ── Interactive / fallback mode: use custom line executor ── */
    char *line = malloc(MAX_CMD_LEN);
    if (!line) {
      LOG_ERROR("malloc: %s", strerror(errno));
      exit(EXIT_FAILURE);
    }

    /* Point stdin at the script that failed to open (already warned above);
     * if it opened but fell through here that can't happen — script_fd < 0
     * is the only path here, so just read terminal. */
    while (1) {
      prompt_for_intput();
      if (!fgets(line, MAX_CMD_LEN, stdin)) break;
      run_command(line);
    }
    free(line);
  }

  LOG_INFO("Init Complete");
  phase = PHASE_GUARDIAN;
  fflush(stdout);

  run_guardian();
  exit(EXIT_SUCCESS);
}
