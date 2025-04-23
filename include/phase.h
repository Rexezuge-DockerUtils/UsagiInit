#pragma once

enum { PHASE_BEGIN, PHASE_SHELL, PHASE_GUARDIAN };

extern int phase;

static inline int *__phase_location(void) { return &phase; }
