#pragma once

#include <sys/types.h>

void handle_child_exit(pid_t pid, int status);
void restart_pending_services(void);
