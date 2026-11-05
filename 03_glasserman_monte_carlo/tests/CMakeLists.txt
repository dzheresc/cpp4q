set(GLASSERMAN_TESTS
    ch01_foundations ch02_random_numbers ch03_sample_paths
    ch04_variance_reduction ch05_quasi_monte_carlo ch06_discretization
    ch07_sensitivities ch08_american_options ch09_risk_management)

foreach(ch IN LISTS GLASSERMAN_TESTS)
  add_executable(glasserman_${ch}_test ${ch}_test.cc)
  target_link_libraries(glasserman_${ch}_test
                        PRIVATE glasserman_${ch} GTest::gtest_main cpp4q_warnings)
  gtest_discover_tests(glasserman_${ch}_test DISCOVERY_TIMEOUT 30)
endforeach()
