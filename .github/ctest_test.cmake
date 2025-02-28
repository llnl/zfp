# We need this CMake versions for tests
cmake_minimum_required(VERSION 3.12..3.15 FATAL_ERROR)

ctest_read_custom_files("${CTEST_BINARY_DIRECTORY}")

# Pick up from where the configure left off.
ctest_start(APPEND)

set(test_exclusions
  # placeholder for tests to exclude provided by the env
  $ENV{CTEST_EXCLUSIONS}
)

string(REPLACE " " ";" test_exclusions "${test_exclusions}")
string(REPLACE ";" "|" test_exclusions "${test_exclusions}")
if (test_exclusions)
  set(test_exclusions EXCLUDE "(${test_exclusions})")
endif ()

set(PARALLEL_LEVEL "10")
if (DEFINED ENV{CTEST_MAX_PARALLELISM})
  set(PARALLEL_LEVEL $ENV{CTEST_MAX_PARALLELISM})
endif()

if (DEFINED ENV{TEST_INCLUSIONS})
  set(test_inclusions INCLUDE $ENV{TEST_INCLUSIONS})
  unset(test_exclusions)
endif()

ctest_test(APPEND
  PARALLEL_LEVEL ${PARALLEL_LEVEL}
  RETURN_VALUE test_result
  ${test_exclusions}
  ${test_inclusions}
)

message(STATUS "ctest_test RETURN_VALUE: ${test_result}")

ctest_submit(PARTS Test Notes)
message(STATUS "Test submission done")

if (test_result)
  message(FATAL_ERROR "Failed to test")
endif ()
