#pragma once

#include <stdio.h>
#include <time.h>

/* shell/preprocess.c internals */
int ends_with_continuation(const char *line, int len);
int find_content_end(const char *line, int len);
int has_amp_before(const char *line, int before_pos);
void transform_line(const char *line, FILE *out);

/* shell/executor.c internals */
char *find_background_operator(char *line);

/* guardian.c internals */
time_t compute_backoff(int restart_count);

#ifdef UNIT_TESTING
/* Test helpers — compiled only when UNIT_TESTING is defined. */
void services_reset(void);
void registration_reset(void);
#endif
