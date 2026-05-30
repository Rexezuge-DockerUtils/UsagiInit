#pragma once

#include <signal.h>

extern int phase;

extern int interactivity;

extern char **usagi_argv;

extern volatile sig_atomic_t shutting_down;
