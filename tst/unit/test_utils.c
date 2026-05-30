#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <stdlib.h>
#include <string.h>

#include "shell/utils.h"

/* Linker-wrapped getenv */
char *__wrap_getenv(const char *name) {
  (void)name;
  return mock_ptr_type(char *);
}

/* ── trim_whitespace ─────────────────────────────────────────────── */

static void test_trim_leading_spaces(void **state) {
  (void)state;
  char s[] = "   hello";
  assert_string_equal(trim_whitespace(s), "hello");
}

static void test_trim_trailing_spaces(void **state) {
  (void)state;
  char s[] = "hello   ";
  assert_string_equal(trim_whitespace(s), "hello");
}

static void test_trim_both_sides(void **state) {
  (void)state;
  char s[] = "  hello world  ";
  assert_string_equal(trim_whitespace(s), "hello world");
}

static void test_trim_tabs(void **state) {
  (void)state;
  char s[] = "\thello\t";
  assert_string_equal(trim_whitespace(s), "hello");
}

static void test_trim_trailing_newline(void **state) {
  (void)state;
  char s[] = "hello\n";
  assert_string_equal(trim_whitespace(s), "hello");
}

static void test_trim_already_clean(void **state) {
  (void)state;
  char s[] = "hello";
  assert_string_equal(trim_whitespace(s), "hello");
}

static void test_trim_empty_string(void **state) {
  (void)state;
  char s[] = "";
  assert_string_equal(trim_whitespace(s), "");
}

static void test_trim_whitespace_only(void **state) {
  (void)state;
  char s[] = "   ";
  assert_string_equal(trim_whitespace(s), "");
}

/* ── expand_variables ────────────────────────────────────────────── */

static void test_expand_found(void **state) {
  (void)state;
  char *val = "/home/testuser";
  will_return(__wrap_getenv, val);
  char *args[] = {"$HOME", NULL};
  expand_variables(args);
  assert_string_equal(args[0], "/home/testuser");
}

static void test_expand_not_found_unchanged(void **state) {
  (void)state;
  will_return(__wrap_getenv, NULL);
  char *args[] = {"$UNDEFINED_VAR_XYZ", NULL};
  expand_variables(args);
  /* When getenv returns NULL, the arg is left unchanged */
  assert_string_equal(args[0], "$UNDEFINED_VAR_XYZ");
}

static void test_expand_no_dollar_untouched(void **state) {
  (void)state;
  /* No getenv call expected — arg does not start with $ */
  char *args[] = {"plainarg", NULL};
  expand_variables(args);
  assert_string_equal(args[0], "plainarg");
}

static void test_expand_multiple_args(void **state) {
  (void)state;
  char *val = "/usr";
  will_return(__wrap_getenv, val); /* called once for $PREFIX */
  char *args[] = {"cmd", "$PREFIX", "arg2", NULL};
  expand_variables(args);
  assert_string_equal(args[0], "cmd");
  assert_string_equal(args[1], "/usr");
  assert_string_equal(args[2], "arg2");
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_trim_leading_spaces),
      cmocka_unit_test(test_trim_trailing_spaces),
      cmocka_unit_test(test_trim_both_sides),
      cmocka_unit_test(test_trim_tabs),
      cmocka_unit_test(test_trim_trailing_newline),
      cmocka_unit_test(test_trim_already_clean),
      cmocka_unit_test(test_trim_empty_string),
      cmocka_unit_test(test_trim_whitespace_only),
      cmocka_unit_test(test_expand_found),
      cmocka_unit_test(test_expand_not_found_unchanged),
      cmocka_unit_test(test_expand_no_dollar_untouched),
      cmocka_unit_test(test_expand_multiple_args),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
