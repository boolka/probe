find_library(CHECK_LIBRARY NAMES check)
find_library(SUBUNIT_LIBRARY NAMES subunit)

add_executable(probe_test ./probe_test.c)
add_executable(probe_cli_test ./probe_cli_test.c)

set_target_properties(
  probe_test
  PROPERTIES
  C_STANDARD 99
  C_STANDARD_REQUIRED ON
)

target_link_libraries(probe_test check subunit probe)
target_link_libraries(probe_cli_test check subunit probe)

add_test(probe_test ./probe_test)
add_test(probe_cli_test ./probe_cli_test)
