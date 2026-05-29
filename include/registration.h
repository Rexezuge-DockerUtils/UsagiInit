#pragma once
#include <sys/types.h>

#define REG_OK    0
#define REG_AGAIN (-1)
#define REG_EOF   (-2)
#define REG_ERROR (-3)

/* Read one service registration from fd (non-blocking).
 * On REG_OK: *pid_out, *argv_out, and *env_out are populated.
 * Caller frees argv_out via free_registration_argv(); env_out ownership
 * is typically transferred to the Service (freed by remove_service). */
int  read_registration(int fd, pid_t *pid_out, char ***argv_out, char ***env_out);
void free_registration_argv(char **argv);
void free_registration_env(char **env);
