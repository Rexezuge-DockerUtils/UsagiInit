#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <sys/wait.h>
#include <unistd.h>

#include "globals.h"
#include "guardian.h"
#include "internal.h"
#include "services.h"

/* Linker-wrapped syscalls — never actually called in these tests */
pid_t __wrap_fork(void) { return mock_type(pid_t); }
int __wrap_execvpe(const char *f, char *const a[], char *const e[]) {
  (void)f;
  (void)a;
  (void)e;
  return mock_type(int);
}
int __wrap_execvp(const char *f, char *const a[]) {
  (void)f;
  (void)a;
  return mock_type(int);
}

static int setup(void **state) {
  (void)state;
  services_reset();
  shutting_down = 0;
  return 0;
}

/* ── compute_backoff ─────────────────────────────────────────────── */

static void test_backoff_zero(void **state) {
  (void)state;
  assert_int_equal(compute_backoff(0), 1);
}

static void test_backoff_one(void **state) {
  (void)state;
  assert_int_equal(compute_backoff(1), 2);
}

static void test_backoff_two(void **state) {
  (void)state;
  assert_int_equal(compute_backoff(2), 4);
}

static void test_backoff_three(void **state) {
  (void)state;
  assert_int_equal(compute_backoff(3), 8);
}

static void test_backoff_four(void **state) {
  (void)state;
  assert_int_equal(compute_backoff(4), 16);
}

static void test_backoff_caps_at_thirty(void **state) {
  (void)state;
  assert_int_equal(compute_backoff(5), 30);
}

static void test_backoff_large_still_thirty(void **state) {
  (void)state;
  assert_int_equal(compute_backoff(100), 30);
}

/* ── handle_child_exit (shutting_down path) ──────────────────────── */

static void test_handle_child_exit_shutting_down_removes_service(void **state) {
  (void)state;
  char *args[] = {"/bin/sleep", "30", NULL};
  add_service(9001, args, NULL);
  assert_int_equal(get_service_count(), 1);

  shutting_down = 1;
  /* Status: exited normally with 0 */
  handle_child_exit(9001, W_EXITCODE(0, 0));

  assert_int_equal(get_service_count(), 0);
}

static void test_handle_child_exit_unknown_pid_noop(void **state) {
  (void)state;
  handle_child_exit(9999, W_EXITCODE(0, 0));
  assert_int_equal(get_service_count(), 0);
}

static void
test_handle_child_exit_normal_removes_without_restart(void **state) {
  (void)state;
  /* Without RESTART_TERMINATED_SERVICES or RESTART_FAILED_SERVICES, a clean
   * exit always removes the service with no scheduling (no fork expected). */
  char *args[] = {"/bin/sleep", "1", NULL};
  add_service(9002, args, NULL);

  handle_child_exit(9002, W_EXITCODE(0, 0));
  assert_int_equal(get_service_count(), 0);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup(test_backoff_zero, setup),
      cmocka_unit_test_setup(test_backoff_one, setup),
      cmocka_unit_test_setup(test_backoff_two, setup),
      cmocka_unit_test_setup(test_backoff_three, setup),
      cmocka_unit_test_setup(test_backoff_four, setup),
      cmocka_unit_test_setup(test_backoff_caps_at_thirty, setup),
      cmocka_unit_test_setup(test_backoff_large_still_thirty, setup),
      cmocka_unit_test_setup(
          test_handle_child_exit_shutting_down_removes_service, setup),
      cmocka_unit_test_setup(test_handle_child_exit_unknown_pid_noop, setup),
      cmocka_unit_test_setup(
          test_handle_child_exit_normal_removes_without_restart, setup),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
