# This file is used to build the project and submit the build results to the dashboard.
ctest_read_custom_files("${CTEST_BINARY_DIRECTORY}")

# Build the project
ctest_start(APPEND)
message(STATUS "CTEST_BUILD_FLAGS: ${CTEST_BUILD_FLAGS}")
ctest_build(APPEND
  NUMBER_WARNINGS num_warnings
  RETURN_VALUE build_result)

# Submit the build results
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.15)
  ctest_submit(PARTS Build BUILD_ID build_id)
  message(STATUS "Build submission build_id: ${build_id}")
else()
  ctest_submit(PARTS Build)
endif()

if (build_result)
  message(FATAL_ERROR "Failed to build")
endif ()
