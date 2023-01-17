#include "probe.h"
#include "test.h"
#include <check.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PROBE_PATH "../app/probe_cli "

START_TEST(probe_cli_args)
{
  ck_assert_int_eq(system(PROBE_PATH "-v"), 0);
  ck_assert_int_eq(system(PROBE_PATH "--version"), 0);
  ck_assert_int_eq(system(PROBE_PATH "-h"), 0);
  ck_assert_int_eq(system(PROBE_PATH "-help"), 0);
  ck_assert_int_eq(system(PROBE_PATH "-p 80 74.125.131.113"), 0);
  ck_assert_int_eq(system(PROBE_PATH "--port=443 74.125.131.113"), 0);
  ck_assert_int_eq(system(PROBE_PATH "-s http 74.125.131.113"), 0);
  ck_assert_int_eq(system(PROBE_PATH "--service=http 74.125.131.113"), 0);
  ck_assert_int_ne(
    system(PROBE_PATH "--timeout=1 --retry=1 --service=pmwebapi localhost"), 0);
  ck_assert_int_ne(system(PROBE_PATH "-t 1 -r 1 -s pmwebapi localhost"), 0);
}

START_TEST(probe_cli_by_ip_port_test)
{
  ck_assert_int_eq(system(PROBE_PATH "-p 80 74.125.131.113"), 0);
  ck_assert_int_eq(system(PROBE_PATH "--port=443 74.125.131.113"), 0);
  ck_assert_int_ne(system(PROBE_PATH "--port=443 255.255.255.255"), 0);
  ck_assert_int_ne(system(PROBE_PATH "--port=443 0.0.0.0"), 0);
}
END_TEST

START_TEST(probe_cli_by_ip_service_test)
{
  ck_assert_int_eq(system(PROBE_PATH "-s http 74.125.131.113"), 0);
  ck_assert_int_eq(system(PROBE_PATH "--service=http 74.125.131.113"), 0);
  ck_assert_int_eq(system(PROBE_PATH "--service=https 74.125.131.113"), 0);
  ck_assert_int_eq(system(PROBE_PATH "--service=https example.com"), 0);
  ck_assert_int_ne(system(PROBE_PATH "--service=4321https1234 74.125.131.113"),
                   0);
}
END_TEST

START_TEST(probe_cli_by_host_port_test)
{
  ck_assert_int_eq(system(PROBE_PATH "--port=80 example.com"), 0);
  ck_assert_int_eq(system(PROBE_PATH "--port=443 example.com"), 0);
  ck_assert_int_ne(system(PROBE_PATH "--port=443 4321example1234.com"), 0);
}
END_TEST

START_TEST(probe_cli_by_host_service_test)
{
  ck_assert_int_eq(system(PROBE_PATH "--port=80 example.com"), 0);
  ck_assert_int_eq(system(PROBE_PATH "--port=443 example.com"), 0);
  ck_assert_int_ne(system(PROBE_PATH "--port=443 4321example1234.com"), 0);
}
END_TEST

START_TEST(probe_cli_config_test)
{
  time_t start, end;

  start = time(NULL);
  ck_assert_int_ne(system(PROBE_PATH "--service=pmwebapi localhost"), 0);
  end = time(NULL);

  ck_assert_int_eq(end - start, (DEFAULT_RETRY_COUNT * DEFAULT_TIMEOUT) - 1);

  start = time(NULL);
  ck_assert_int_ne(
    system(PROBE_PATH "--timeout=2 --retry=5 --service=pmwebapi localhost"), 0);
  end = time(NULL);

  ck_assert_int_eq(end - start, (5 * 2) - 2);
}
END_TEST

int
main()
{
  uint32_t number_failed;
  SRunner* sr;
  Suite* s;
  TCase* t;

  s = suite_create("Probe lib test suite");
  t = tcase_create("API");

  tcase_add_test(t, probe_cli_args);
  tcase_add_test(t, probe_cli_by_ip_port_test);
  tcase_add_test(t, probe_cli_by_ip_service_test);
  tcase_add_test(t, probe_cli_by_host_port_test);
  tcase_add_test(t, probe_cli_by_host_service_test);
  tcase_add_test(t, probe_cli_config_test);
  tcase_set_timeout(t, TEST_CASE_TIMEOUT);
  suite_add_tcase(s, t);

  sr = srunner_create(s);

#ifdef DEBUG
  srunner_set_fork_status(sr, CK_NOFORK);
#endif

  srunner_run_all(sr, CK_SUBUNIT);

  number_failed = srunner_ntests_failed(sr);

  srunner_free(sr);

  return number_failed;
}
