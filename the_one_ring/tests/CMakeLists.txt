find_package(GTest REQUIRED)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")

if(NOT TEST_SOURCES)
  message(STATUS "No test sources found in tests/ — skipping test target.")
  return()
endif()

add_executable(${PROJECT_NAME}_tests ${TEST_SOURCES})

target_include_directories(${PROJECT_NAME}_tests
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>)

target_compile_features(${PROJECT_NAME}_tests PRIVATE cxx_std_20)

target_link_libraries(${PROJECT_NAME}_tests
  PRIVATE
    GTest::gtest
    GTest::gtest_main
    rt
    pthread)

# Sanitizers and coverage propagate from the parent CMakeCache flags.
apply_sanitizers(${PROJECT_NAME}_tests)

if(ENABLE_COVERAGE)
  apply_coverage(${PROJECT_NAME}_tests)
  setup_coverage_targets(${PROJECT_NAME}_tests)
endif()

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_tests
  PROPERTIES TIMEOUT 30)
