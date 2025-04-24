#include "shell/parser.h"
#include <stdbool.h>
#include <string.h>

void parse_command(char *line, char **args) {
  int i = 0;
  bool in_quote = false;
  char *start = line;
  char *current = line;

  while (*current && i < MAX_ARGS - 1) {
    if (*current == '"') {
      in_quote = !in_quote;
      if (!in_quote) {
        // End of quoted section
        *current = '\0';
        args[i++] = start;
        start = current + 1;
      } else {
        // Start of quoted section
        start = current + 1;
      }
      current++;
    } else if (!in_quote &&
               (*current == ' ' || *current == '\t' || *current == '\n')) {
      if (current != start) {
        *current = '\0';
        args[i++] = start;
      }
      current++;
      start = current;
    } else {
      current++;
    }
  }

  // Add the last argument if there's any
  if (current != start && i < MAX_ARGS - 1) {
    args[i++] = start;
  }

  args[i] = NULL;
}
