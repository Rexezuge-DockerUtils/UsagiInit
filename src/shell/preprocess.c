#include "shell/preprocess.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Find the index of the last significant character in line, excluding inline
 * comments and trailing whitespace.  Returns -1 for blank/comment lines.
 * Tracks double- and single-quote state to avoid mis-identifying quoted text. */
static int find_content_end(const char *line, int len) {
  int dq = 0, sq = 0, last = -1;
  for (int i = 0; i < len; i++) {
    char c = line[i];
    if (sq) {
      if (c == '\'') sq = 0;
      last = i;
    } else if (dq) {
      if (c == '\\' && i + 1 < len) { last = i; i++; last = i; }
      else if (c == '"') { dq = 0; last = i; }
      else last = i;
    } else {
      if (c == '\'') { sq = 1; last = i; }
      else if (c == '"') { dq = 1; last = i; }
      else if (c == '#') break;
      else if (c != ' ' && c != '\t' && c != '\n') last = i;
    }
  }
  return last;
}

/* Return 1 if there is an unquoted '&' in line[0..before_pos). */
static int has_amp_before(const char *line, int before_pos) {
  int dq = 0, sq = 0;
  for (int i = 0; i < before_pos; i++) {
    char c = line[i];
    if (sq) { if (c == '\'') sq = 0; }
    else if (dq) {
      if (c == '\\' && i + 1 < before_pos) i++;
      else if (c == '"') dq = 0;
    } else {
      if (c == '\'') sq = 1;
      else if (c == '"') dq = 1;
      else if (c == '#') break;
      else if (c == '&') return 1;
    }
  }
  return 0;
}

static void transform_line(const char *line, FILE *out) {
  int len  = (int)strlen(line);
  int last = find_content_end(line, len);

  /* Service line: last significant char is a lone '&' (not '&&') with no
   * other '&' earlier on the line (avoids matching `cmd1 & cmd2 &`). */
  if (last >= 0 && line[last] == '&' &&
      (last == 0 || line[last - 1] != '&') &&
      !has_amp_before(line, last)) {

    /* Find end of command text (trim whitespace before '&'). */
    int cmd_end = last - 1;
    while (cmd_end >= 0 && (line[cmd_end] == ' ' || line[cmd_end] == '\t'))
      cmd_end--;

    if (cmd_end >= 0) {
      fputs("usagi-reg ", out);
      fwrite(line, 1, cmd_end + 1, out);
      fputc('\n', out);
      return;
    }
  }

  fputs(line, out);
}

char *preprocess_script(const char *input_path) {
  FILE *in = fopen(input_path, "r");
  if (!in) return NULL;

  char tmp_path[] = "/tmp/usagi-init-XXXXXX";
  int  tmp_fd     = mkstemp(tmp_path);
  if (tmp_fd < 0) { fclose(in); return NULL; }

  FILE *out = fdopen(tmp_fd, "w");
  if (!out) { close(tmp_fd); fclose(in); return NULL; }

  char line[4096];
  while (fgets(line, sizeof(line), in))
    transform_line(line, out);

  fclose(in);
  fclose(out);
  return strdup(tmp_path);
}
