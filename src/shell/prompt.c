#include "shell/prompt.h"
#include "gVariables.h"
#include "types.h"

#include <stdio.h>

void prompt_for_intput(void) {
  if (interactivity == INTERACTIVE) {
    printf("$> ");
    fflush(stdout);
  }
}
