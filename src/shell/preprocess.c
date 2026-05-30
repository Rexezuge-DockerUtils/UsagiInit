#include "shell/preprocess.h"
#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Return 1 if the physical line ends with a continuation backslash (an odd
 * run of backslashes before the newline, outside of single quotes). */
int ends_with_continuation(const char *line, int len) {
  int end = len;
  if (end > 0 && line[end - 1] == '\n')
    end--;
  if (end == 0 || line[end - 1] != '\\')
    return 0;

  int bs = 0, i = end - 1;
  while (i >= 0 && line[i] == '\\') {
    bs++;
    i--;
  }
  if (bs % 2 == 0)
    return 0;

  int sq = 0, dq = 0;
  for (int j = 0; j < end; j++) {
    char c = line[j];
    if (sq) {
      if (c == '\'')
        sq = 0;
    } else if (dq) {
      if (c == '\\' && j + 1 < end)
        j++;
      else if (c == '"')
        dq = 0;
    } else {
      if (c == '\'')
        sq = 1;
      else if (c == '"')
        dq = 1;
      else if (c == '\\' && j + 1 < end)
        j++;
    }
  }
  return !sq;
}

/* Find the index of the last significant character in line, excluding inline
 * comments and trailing whitespace.  Returns -1 for blank/comment lines.
 * Tracks double- and single-quote state to avoid mis-identifying quoted text.
 */
int find_content_end(const char *line, int len) {
  int dq = 0, sq = 0, last = -1;
  for (int i = 0; i < len; i++) {
    char c = line[i];
    if (sq) {
      if (c == '\'')
        sq = 0;
      last = i;
    } else if (dq) {
      if (c == '\\' && i + 1 < len) {
        last = i;
        i++;
        last = i;
      } else if (c == '"') {
        dq = 0;
        last = i;
      } else
        last = i;
    } else {
      if (c == '\'') {
        sq = 1;
        last = i;
      } else if (c == '"') {
        dq = 1;
        last = i;
      } else if (c == '#')
        break;
      else if (c != ' ' && c != '\t' && c != '\n')
        last = i;
    }
  }
  return last;
}

/* Return 1 if there is an unquoted '&' in line[0..before_pos). */
int has_amp_before(const char *line, int before_pos) {
  int dq = 0, sq = 0;
  for (int i = 0; i < before_pos; i++) {
    char c = line[i];
    if (sq) {
      if (c == '\'')
        sq = 0;
    } else if (dq) {
      if (c == '\\' && i + 1 < before_pos)
        i++;
      else if (c == '"')
        dq = 0;
    } else {
      if (c == '\'')
        sq = 1;
      else if (c == '"')
        dq = 1;
      else if (c == '#')
        break;
      else if (c == '&') {
        /* &N is fd duplication in a redirection (e.g. 2>&1) — not a
         * background operator; skip it and keep scanning. */
        if (i + 1 < before_pos && line[i + 1] >= '0' && line[i + 1] <= '9') {
          i++;
          continue;
        }
        return 1;
      }
    }
  }
  return 0;
}

void transform_line(const char *line, FILE *out) {
  int len = (int)strlen(line);
  int last = find_content_end(line, len);

  /* Service line: last significant char is a lone '&' (not '&&') with no
   * other '&' earlier on the line (avoids matching `cmd1 & cmd2 &`). */
  if (last >= 0 && line[last] == '&' && (last == 0 || line[last - 1] != '&') &&
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
  if (!in)
    return NULL;

  char tmp_path[] = "/tmp/usagi-init-XXXXXX";
  int tmp_fd = mkstemp(tmp_path);
  if (tmp_fd < 0) {
    fclose(in);
    return NULL;
  }

  FILE *out = fdopen(tmp_fd, "w");
  if (!out) {
    close(tmp_fd);
    fclose(in);
    return NULL;
  }

  size_t buf_cap = 4096;
  size_t buf_len = 0;
  char *buf = malloc(buf_cap);
  if (!buf) {
    fclose(in);
    fclose(out);
    return NULL;
  }

  char line[4096];
  while (fgets(line, sizeof(line), in)) {
    int llen = (int)strlen(line);
    int cont = ends_with_continuation(line, llen);

    /* For continuation lines, strip the trailing backslash (and newline if
     * present) so the next physical line joins without a line break. */
    int end = llen;
    if (end > 0 && line[end - 1] == '\n')
      end--;
    int copy = cont ? end - 1 : llen;

    if (buf_len + (size_t)copy + 1 > buf_cap) {
      buf_cap = (buf_len + (size_t)copy + 1) * 2;
      char *tmp = realloc(buf, buf_cap);
      if (!tmp) {
        free(buf);
        fclose(in);
        fclose(out);
        return NULL;
      }
      buf = tmp;
    }

    memcpy(buf + buf_len, line, (size_t)copy);
    buf_len += (size_t)copy;
    buf[buf_len] = '\0';

    if (!cont) {
      transform_line(buf, out);
      buf_len = 0;
    }
  }

  if (buf_len > 0)
    transform_line(buf, out);

  free(buf);
  fclose(in);
  fclose(out);
  return strdup(tmp_path);
}
