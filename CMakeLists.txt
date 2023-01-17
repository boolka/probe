cmake_minimum_required(VERSION 3.0)
project(probe C)

set(C_STANDARD 99)
set(C_STANDARD_REQUIRED true)

# set(BUILD_TYPE DEBUG)

if (BUILD_TYPE MATCHES DEBUG)
  add_definitions(-DDEBUG)
  set(CMAKE_BUILD_TYPE Debug)
endif()

enable_testing()
include_directories(src)

add_subdirectory(test)
add_subdirectory(src app)
