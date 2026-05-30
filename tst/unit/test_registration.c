#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "internal.h"
#include "registration.h"

static int setup(void **state) {
  (void)state;
  registration_reset();
  return 0;
}

/* Write a complete registration message to the write end of a pipe and close
 * it. */
static void write_registration(int wfd, pid_t pid, const char **argv, int argc,
                               const char **env, int envc) {
  dprintf(wfd, "%d\n%d\n", (int)pid, argc);
  for (int i = 0; i < argc; i++)
    dprintf(wfd, "%s\n", argv[i]);
  dprintf(wfd, "%d\n", envc);
  for (int i = 0; i < envc; i++)
    dprintf(wfd, "%s\n", env[i]);
  close(wfd);
}

static void test_valid_registration_no_env(void **state) {
  (void)state;
  int fds[2];
  assert_int_equal(pipe(fds), 0);

  const char *argv[] = {"/bin/sleep", "30"};
  write_registration(fds[1], 1234, argv, 2, NULL, 0);

  pid_t pid;
  char **out_argv, **out_env;
  int r = read_registration(fds[0], &pid, &out_argv, &out_env);
  close(fds[0]);

  assert_int_equal(r, REG_OK);
  assert_int_equal((int)pid, 1234);
  assert_string_equal(out_argv[0], "/bin/sleep");
  assert_string_equal(out_argv[1], "30");
  assert_null(out_argv[2]);
  assert_null(out_env[0]); /* envc=0 → empty env array */

  free_registration_argv(out_argv);
  free_registration_env(out_env);
}

static void test_valid_registration_with_env(void **state) {
  (void)state;
  int fds[2];
  assert_int_equal(pipe(fds), 0);

  const char *argv[] = {"/usr/bin/nginx"};
  const char *env[] = {"HOME=/root", "PATH=/usr/bin"};
  write_registration(fds[1], 5678, argv, 1, env, 2);

  pid_t pid;
  char **out_argv, **out_env;
  int r = read_registration(fds[0], &pid, &out_argv, &out_env);
  close(fds[0]);

  assert_int_equal(r, REG_OK);
  assert_int_equal((int)pid, 5678);
  assert_string_equal(out_argv[0], "/usr/bin/nginx");
  assert_null(out_argv[1]);
  assert_string_equal(out_env[0], "HOME=/root");
  assert_string_equal(out_env[1], "PATH=/usr/bin");
  assert_null(out_env[2]);

  free_registration_argv(out_argv);
  free_registration_env(out_env);
}

static void test_eof_on_empty_pipe(void **state) {
  (void)state;
  int fds[2];
  assert_int_equal(pipe(fds), 0);
  close(fds[1]); /* close write end immediately — EOF */

  pid_t pid;
  char **out_argv, **out_env;
  int r = read_registration(fds[0], &pid, &out_argv, &out_env);
  close(fds[0]);

  assert_int_equal(r, REG_EOF);
}

static void test_invalid_argc_zero(void **state) {
  (void)state;
  int fds[2];
  assert_int_equal(pipe(fds), 0);

  /* argc=0 is invalid */
  dprintf(fds[1], "999\n0\n");
  close(fds[1]);

  pid_t pid;
  char **out_argv, **out_env;
  int r = read_registration(fds[0], &pid, &out_argv, &out_env);
  close(fds[0]);

  assert_int_equal(r, REG_ERROR);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup(test_valid_registration_no_env, setup),
      cmocka_unit_test_setup(test_valid_registration_with_env, setup),
      cmocka_unit_test_setup(test_eof_on_empty_pipe, setup),
      cmocka_unit_test_setup(test_invalid_argc_zero, setup),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
