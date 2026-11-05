set(ARMSTRONG_TESTS
    ch01_getting_started ch02_data_types ch03_functions ch04_flow_of_control
    ch05_multiple_files ch06_unit_testing ch07_using_classes
    ch08_user_defined_types ch09_monte_carlo ch10_interfaces
    ch11_arrays_strings_pointers ch12_sophisticated_classes ch13_portfolio
    ch14_delta_hedging ch15_debugging_tools ch16_matrix ch17_templates
    ch18_stl ch19_function_objects ch20_threads ch21_next_steps
    appendix_a_risk_neutral)

foreach(ch IN LISTS ARMSTRONG_TESTS)
  add_executable(armstrong_${ch}_test ${ch}_test.cc)
  target_link_libraries(armstrong_${ch}_test
                        PRIVATE armstrong_${ch} GTest::gtest_main cpp4q_warnings)
  gtest_discover_tests(armstrong_${ch}_test DISCOVERY_TIMEOUT 30)
endforeach()

# The PDE and binomial pricers are cross-checked against each other.
target_link_libraries(armstrong_ch21_next_steps_test
                      PRIVATE armstrong_appendix_a_risk_neutral)
