#pragma once

#include "types.h"

#include <unistd.h>

// Runs the executable with redirections. Returns the child's PID. Does NOT wait
// for it.
pid_t run_executable(const command_t *cmd);
