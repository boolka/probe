#include "probe.h"
#include "test.h"
#include <check.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

START_TEST(probe_version_test)
{
  char* p_ver = probe_version();
  ck_assert_ptr_nonnull(p_ver);
  ck_assert_str_eq(p_ver, "0.1.0");
}

START_TEST(ipv4_service_probe_test)
{
  ck_assert_int_eq(ipv4_service_probe("216.58.210.174", "http", NULL),
                   AVAILABLE);
  ck_assert_int_eq(ipv4_service_probe("216.58.210.174", "https", NULL),
                   AVAILABLE);
  ck_assert_int_eq(ipv4_service_probe("0.0.0.0", "https", NULL), UNKNOWN_HOST);
  ck_assert_int_eq(ipv4_service_probe("216.58.210.174", "4321https1234", NULL),
                   UNKNOWN_SERVICE);
}
END_TEST

START_TEST(ipv4_port_probe_test)
{
  ck_assert_int_eq(ipv4_port_probe("216.58.210.174", 80, NULL), AVAILABLE);
  ck_assert_int_eq(ipv4_port_probe("216.58.210.174", 443, NULL), AVAILABLE);
  ck_assert_int_eq(ipv4_port_probe("1.2.3.999", 443, NULL), INVALID_IP);
}
END_TEST

START_TEST(host_port_probe_test)
{
  ck_assert_int_eq(host_port_probe("example.com", 80, NULL), AVAILABLE);
  ck_assert_int_eq(host_port_probe("one.one.one.one", 53, "4321udp1234"),
                   UNKNOWN_PROTOCOL);
  ck_assert_int_eq(host_port_probe("example.com", 443, "tcp"), AVAILABLE);
  ck_assert_int_eq(host_port_probe("4321example1234.com", 443, NULL),
                   UNKNOWN_HOST);
}
END_TEST

START_TEST(host_service_probe_test)
{
  ck_assert_int_eq(host_service_probe("example.com", "http", NULL), AVAILABLE);
  ck_assert_int_eq(host_service_probe("example.com", "https", NULL), AVAILABLE);
  ck_assert_int_eq(host_service_probe("4321example1234.com", "https", NULL),
                   UNKNOWN_HOST);
  ck_assert_int_eq(host_service_probe("example.com", "4321https1234", NULL),
                   UNKNOWN_SERVICE);
}
END_TEST

START_TEST(timeout_retry_adjust_test)
{
  time_t start, end;
  // Check that this service is currently unavailable on host machine
  start = time(NULL);
  ck_assert_int_eq(host_service_probe("localhost", "nimspooler", NULL),
                   UNAVAILABLE);
  end = time(NULL);

  ck_assert_int_eq(end - start, (DEFAULT_RETRY_COUNT * DEFAULT_TIMEOUT) - 1);

  probe_config(3, 2);

  start = time(NULL);
  ck_assert_int_eq(host_service_probe("localhost", "nimspooler", NULL),
                   UNAVAILABLE);
  end = time(NULL);

  ck_assert_int_eq(end - start, (3 * 2) - 2);
}
END_TEST

uint32_t
main()
{
  uint32_t number_failed;
  SRunner* sr;
  Suite* s;
  TCase* t;

  s = suite_create("Probe lib test suite");
  t = tcase_create("API");

  tcase_add_test(t, probe_version_test);
  tcase_add_test(t, ipv4_port_probe_test);
  tcase_add_test(t, ipv4_service_probe_test);
  tcase_add_test(t, host_port_probe_test);
  tcase_add_test(t, host_service_probe_test);
  tcase_add_test(t, timeout_retry_adjust_test);
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
