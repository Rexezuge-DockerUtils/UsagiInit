#include "signals.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

static void signal_handler(int signo) {
  // Forward signal to the entire process group
  if (signo == SIGINT || signo == SIGTERM || signo == SIGHUP) {
    if (kill(-1, signo) < 0) {
      perror("kill (signal forwarding)");
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
