#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <stdio.h>
#include <string.h>

#include "shell/parser.h"

static void test_parse_simple_two_args(void **state) {
  (void)state;
  char line[] = "ls -la";
  char *args[MAX_ARGS];
  parse_command(line, args);
  assert_string_equal(args[0], "ls");
  assert_string_equal(args[1], "-la");
  assert_null(args[2]);
}

static void test_parse_double_quoted_space(void **state) {
  (void)state;
  char line[] = "echo \"hello world\"";
  char *args[MAX_ARGS];
  parse_command(line, args);
  assert_string_equal(args[0], "echo");
  assert_string_equal(args[1], "hello world");
  assert_null(args[2]);
}

static void test_parse_single_arg(void **state) {
  (void)state;
  char line[] = "sleep";
  char *args[MAX_ARGS];
  parse_command(line, args);
  assert_string_equal(args[0], "sleep");
  assert_null(args[1]);
}

static void test_parse_empty_string(void **state) {
  (void)state;
  char line[] = "";
  char *args[MAX_ARGS];
  parse_command(line, args);
  assert_null(args[0]);
}

static void test_parse_whitespace_only(void **state) {
  (void)state;
  char line[] = "   ";
  char *args[MAX_ARGS];
  parse_command(line, args);
  assert_null(args[0]);
}

static void test_parse_multiple_spaces(void **state) {
  (void)state;
  char line[] = "cmd   arg1   arg2";
  char *args[MAX_ARGS];
  parse_command(line, args);
  assert_string_equal(args[0], "cmd");
  assert_string_equal(args[1], "arg1");
  assert_string_equal(args[2], "arg2");
  assert_null(args[3]);
}

static void test_parse_tabs_as_delimiters(void **state) {
  (void)state;
  char line[] = "cmd\targ";
  char *args[MAX_ARGS];
  parse_command(line, args);
  assert_string_equal(args[0], "cmd");
  assert_string_equal(args[1], "arg");
  assert_null(args[2]);
}

static void test_parse_many_args(void **state) {
  (void)state;
  /* Build a line with MAX_ARGS-1 space-separated tokens */
  char line[MAX_ARGS * 3];
  int pos = 0;
  for (int i = 0; i < MAX_ARGS - 1; i++) {
    pos += snprintf(line + pos, sizeof(line) - pos, "a%d ", i);
  }
  char *args[MAX_ARGS];
  parse_command(line, args);
  /* Verify termination — should not overflow the array */
  int count = 0;
  while (args[count] != NULL)
    count++;
  assert_true(count <= MAX_ARGS - 1);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_parse_simple_two_args),
      cmocka_unit_test(test_parse_double_quoted_space),
      cmocka_unit_test(test_parse_single_arg),
      cmocka_unit_test(test_parse_empty_string),
      cmocka_unit_test(test_parse_whitespace_only),
      cmocka_unit_test(test_parse_multiple_spaces),
      cmocka_unit_test(test_parse_tabs_as_delimiters),
      cmocka_unit_test(test_parse_many_args),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
