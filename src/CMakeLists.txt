add_library(probe STATIC probe.c)

set_target_properties(
  probe PROPERTIES
  C_STANDARD 99
  C_STANDARD_REQUIRED ON
)

add_executable(probe_cli ./main.c)

target_link_libraries(probe_cli probe)
