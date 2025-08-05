#include "shell/prompt.h"
#include "globals.h"
#include "types.h"

#include <stdio.h>

void prompt_for_intput(void) {
  if (interactivity == INTERACTIVE) {
    printf("$> ");
    fflush(stdout);
  }
}
