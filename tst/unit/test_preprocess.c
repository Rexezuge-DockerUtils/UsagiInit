#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <stdio.h>
#include <string.h>

#include "internal.h"

/* ── has_amp_before ────────────────────────────────────────────── */

static void test_has_amp_no_amp(void **state) {
  (void)state;
  assert_int_equal(has_amp_before("cmd arg", 7), 0);
}

static void test_has_amp_standalone_before_pos(void **state) {
  (void)state;
  /* "cmd1 & cmd2 &" — check for amp before the trailing & (pos 13) */
  const char *line = "cmd1 & cmd2 &";
  int last = (int)strlen(line) - 1; /* index of trailing & */
  assert_int_equal(has_amp_before(line, last), 1);
}

static void test_has_amp_fd_dup_not_counted(void **state) {
  (void)state;
  /* "cmd > /dev/null 2>&1 &" — the & in 2>&1 must not trigger */
  const char *line = "cmd > /dev/null 2>&1 &";
  int last = (int)strlen(line) - 1;
  assert_int_equal(has_amp_before(line, last), 0);
}

static void test_has_amp_in_single_quotes(void **state) {
  (void)state;
  const char *line = "cmd '&' &";
  int last = (int)strlen(line) - 1;
  assert_int_equal(has_amp_before(line, last), 0);
}

static void test_has_amp_in_double_quotes(void **state) {
  (void)state;
  const char *line = "cmd \"&\" &";
  int last = (int)strlen(line) - 1;
  assert_int_equal(has_amp_before(line, last), 0);
}

static void test_has_amp_and_and(void **state) {
  (void)state;
  /* "cmd1 && cmd2 &" — first & of && is not followed by a digit, returns 1 */
  const char *line = "cmd1 && cmd2 &";
  int last = (int)strlen(line) - 1;
  assert_int_equal(has_amp_before(line, last), 1);
}

static void test_has_amp_empty(void **state) {
  (void)state;
  assert_int_equal(has_amp_before("&", 0),
                   0); /* before_pos=0, nothing to scan */
}

/* ── ends_with_continuation ─────────────────────────────────────── */

static void test_continuation_single_backslash(void **state) {
  (void)state;
  /* "cmd \\\n" in C: cmd + space + backslash + newline */
  const char line[] = "cmd \\\n";
  assert_int_equal(ends_with_continuation(line, (int)sizeof(line) - 1), 1);
}

static void test_continuation_no_backslash(void **state) {
  (void)state;
  const char line[] = "cmd\n";
  assert_int_equal(ends_with_continuation(line, (int)sizeof(line) - 1), 0);
}

static void test_continuation_double_backslash(void **state) {
  (void)state;
  /* "\\\\" is two backslashes — even count, not a continuation */
  const char line[] = "cmd \\\\\n";
  assert_int_equal(ends_with_continuation(line, (int)sizeof(line) - 1), 0);
}

static void test_continuation_backslash_in_single_quotes(void **state) {
  (void)state;
  /* "'\\" — open single-quote then backslash+newline: sq=1 at end → !sq = 0 */
  const char really_inside[] = "'\\\n";
  assert_int_equal(
      ends_with_continuation(really_inside, (int)sizeof(really_inside) - 1), 0);
}

/* ── find_content_end ───────────────────────────────────────────── */

static void test_find_content_end_normal(void **state) {
  (void)state;
  const char *line = "hello world";
  int last = find_content_end(line, (int)strlen(line));
  assert_int_equal(last, 10); /* 'd' at index 10 */
}

static void test_find_content_end_trailing_whitespace(void **state) {
  (void)state;
  const char *line = "hello   ";
  int last = find_content_end(line, (int)strlen(line));
  assert_int_equal(last, 4); /* 'o' at index 4 */
}

static void test_find_content_end_comment(void **state) {
  (void)state;
  const char *line = "cmd # comment";
  int last = find_content_end(line, (int)strlen(line));
  assert_int_equal(last, 2); /* 'd' at index 2 */
}

static void test_find_content_end_blank(void **state) {
  (void)state;
  const char *line = "   \n";
  int last = find_content_end(line, (int)strlen(line));
  assert_int_equal(last, -1);
}

static void test_find_content_end_comment_only(void **state) {
  (void)state;
  const char *line = "# just a comment\n";
  int last = find_content_end(line, (int)strlen(line));
  assert_int_equal(last, -1);
}

/* ── transform_line ─────────────────────────────────────────────── */

static char *read_fmemstream(const char *line) {
  static char buf[1024];
  memset(buf, 0, sizeof(buf));
  FILE *out = fmemopen(buf, sizeof(buf), "w");
  transform_line(line, out);
  fclose(out);
  return buf;
}

static void test_transform_simple_service(void **state) {
  (void)state;
  assert_string_equal(read_fmemstream("./svc &\n"), "usagi-reg ./svc\n");
}

static void test_transform_service_with_redirect(void **state) {
  (void)state;
  assert_string_equal(read_fmemstream("./svc > /dev/null 2>&1 &\n"),
                      "usagi-reg ./svc > /dev/null 2>&1\n");
}

static void test_transform_service_with_args(void **state) {
  (void)state;
  assert_string_equal(read_fmemstream("/bin/sleep 30 &\n"),
                      "usagi-reg /bin/sleep 30\n");
}

static void test_transform_not_a_service(void **state) {
  (void)state;
  /* No trailing & — written unchanged */
  assert_string_equal(read_fmemstream("./svc arg\n"), "./svc arg\n");
}

static void test_transform_comment_line(void **state) {
  (void)state;
  assert_string_equal(read_fmemstream("# comment\n"), "# comment\n");
}

static void test_transform_multi_amp_not_transformed(void **state) {
  (void)state;
  /* cmd1 & cmd2 & — has_amp_before returns 1, so not transformed */
  assert_string_equal(read_fmemstream("cmd1 & cmd2 &\n"), "cmd1 & cmd2 &\n");
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_has_amp_no_amp),
      cmocka_unit_test(test_has_amp_standalone_before_pos),
      cmocka_unit_test(test_has_amp_fd_dup_not_counted),
      cmocka_unit_test(test_has_amp_in_single_quotes),
      cmocka_unit_test(test_has_amp_in_double_quotes),
      cmocka_unit_test(test_has_amp_and_and),
      cmocka_unit_test(test_has_amp_empty),
      cmocka_unit_test(test_continuation_single_backslash),
      cmocka_unit_test(test_continuation_no_backslash),
      cmocka_unit_test(test_continuation_double_backslash),
      cmocka_unit_test(test_continuation_backslash_in_single_quotes),
      cmocka_unit_test(test_find_content_end_normal),
      cmocka_unit_test(test_find_content_end_trailing_whitespace),
      cmocka_unit_test(test_find_content_end_comment),
      cmocka_unit_test(test_find_content_end_blank),
      cmocka_unit_test(test_find_content_end_comment_only),
      cmocka_unit_test(test_transform_simple_service),
      cmocka_unit_test(test_transform_service_with_redirect),
      cmocka_unit_test(test_transform_service_with_args),
      cmocka_unit_test(test_transform_not_a_service),
      cmocka_unit_test(test_transform_comment_line),
      cmocka_unit_test(test_transform_multi_amp_not_transformed),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
