#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <limits.h>
#include <string.h>
#include <time.h>

#include "internal.h"
#include "services.h"

static int setup(void **state) {
  (void)state;
  services_reset();
  return 0;
}

static void test_add_and_find(void **state) {
  (void)state;
  char *args[] = {"sleep", "30", NULL};
  assert_int_equal(add_service(100, args, NULL), 0);
  assert_int_equal(get_service_count(), 1);
  Service *s = find_service(100);
  assert_non_null(s);
  assert_int_equal(s->pid, 100);
}

static void test_find_unknown_pid(void **state) {
  (void)state;
  assert_null(find_service(9999));
}

static void test_remove_decrements_count(void **state) {
  (void)state;
  char *args[] = {"sleep", NULL};
  add_service(200, args, NULL);
  assert_int_equal(get_service_count(), 1);
  remove_service(200);
  assert_int_equal(get_service_count(), 0);
}

static void test_remove_unknown_pid_noop(void **state) {
  (void)state;
  /* Should not crash or change the count */
  char *args[] = {"sleep", NULL};
  add_service(300, args, NULL);
  remove_service(9999);
  assert_int_equal(get_service_count(), 1);
}

static void test_multiple_services(void **state) {
  (void)state;
  char *a1[] = {"svc1", NULL};
  char *a2[] = {"svc2", NULL};
  char *a3[] = {"svc3", NULL};
  add_service(1, a1, NULL);
  add_service(2, a2, NULL);
  add_service(3, a3, NULL);
  assert_int_equal(get_service_count(), 3);
  assert_non_null(find_service(2));
  remove_service(2);
  assert_int_equal(get_service_count(), 2);
  assert_null(find_service(2));
  assert_non_null(find_service(1));
  assert_non_null(find_service(3));
}

static void test_get_next_restart_time_none_pending(void **state) {
  (void)state;
  /* No services with next_restart set — should return 0 */
  char *args[] = {"sleep", NULL};
  add_service(400, args, NULL);
  assert_int_equal(get_next_restart_time(), 0);
}

static void test_get_next_restart_time_with_pending(void **state) {
  (void)state;
  char *args[] = {"sleep", NULL};
  add_service(500, args, NULL);
  Service *s = find_service(500);
  s->next_restart = 12345;
  assert_int_equal(get_next_restart_time(), 12345);
}

static void test_get_service_pending_restart(void **state) {
  (void)state;
  char *args[] = {"sleep", NULL};
  add_service(600, args, NULL);
  Service *s = find_service(600);
  time_t now = time(NULL);
  s->next_restart = now - 1; /* due in the past */
  Service *pending = get_service_pending_restart(now);
  assert_non_null(pending);
  assert_int_equal(pending->pid, 600);
}

static void test_get_service_pending_restart_not_yet(void **state) {
  (void)state;
  char *args[] = {"sleep", NULL};
  add_service(700, args, NULL);
  Service *s = find_service(700);
  time_t now = time(NULL);
  s->next_restart = now + 9999; /* far in the future */
  assert_null(get_service_pending_restart(now));
}

static void test_add_service_count_zero_initially(void **state) {
  (void)state;
  assert_int_equal(get_service_count(), 0);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup(test_add_service_count_zero_initially, setup),
      cmocka_unit_test_setup(test_add_and_find, setup),
      cmocka_unit_test_setup(test_find_unknown_pid, setup),
      cmocka_unit_test_setup(test_remove_decrements_count, setup),
      cmocka_unit_test_setup(test_remove_unknown_pid_noop, setup),
      cmocka_unit_test_setup(test_multiple_services, setup),
      cmocka_unit_test_setup(test_get_next_restart_time_none_pending, setup),
      cmocka_unit_test_setup(test_get_next_restart_time_with_pending, setup),
      cmocka_unit_test_setup(test_get_service_pending_restart, setup),
      cmocka_unit_test_setup(test_get_service_pending_restart_not_yet, setup),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
