#include "signals.h"
#include "phase.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

static void signal_handler(int signo) {
  // Forward signal to the entire process group
  if (signo == SIGINT || signo == SIGTERM || signo == SIGHUP) {
    kill(-1, signo);
    if (phase == PHASE_GUARDIAN) {
      exit(EXIT_SUCCESS);
    } else if (phase == PHASE_SHELL) {
      printf("\n$> ");
      fflush(stdout);
      tcflush(STDIN_FILENO, TCIFLUSH);
    }
  }
}

void setup_signal_forwarding(void) {
  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGINT, &sa, NULL) < 0 || sigaction(SIGTERM, &sa, NULL) < 0 ||
      sigaction(SIGHUP, &sa, NULL) < 0) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
}
