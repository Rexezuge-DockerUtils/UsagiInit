#pragma once
#include <sys/types.h>

#define REG_OK    0
#define REG_AGAIN (-1)
#define REG_EOF   (-2)
#define REG_ERROR (-3)

/* Read one service registration from fd (non-blocking).
 * On REG_OK: *pid_out and *argv_out are populated; caller frees argv_out
 * via free_registration_argv(). */
int  read_registration(int fd, pid_t *pid_out, char ***argv_out);
void free_registration_argv(char **argv);
