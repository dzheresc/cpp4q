set(JOSHI_TESTS
    appendix_a_black_scholes appendix_b_distributions appendix_c_array
    ch01_simple_monte_carlo ch02_encapsulation ch03_inheritance
    ch04_virtual_constructor ch05_strategies_statistics ch06_random_numbers
    ch07_exotics_engine ch08_trees ch09_solvers_implied_vol ch10_factory
    ch11_patterns_revisited ch12_situation_2007 ch13_exceptions
    ch14_template_factory ch15_excel_interface ch16_decoupling)

foreach(ch IN LISTS JOSHI_TESTS)
  add_executable(joshi_${ch}_test ${ch}_test.cc)
  target_link_libraries(joshi_${ch}_test
                        PRIVATE joshi_${ch} GTest::gtest_main cpp4q_warnings)
  gtest_discover_tests(joshi_${ch}_test DISCOVERY_TIMEOUT 30)
endforeach()

target_link_libraries(joshi_ch14_template_factory_test PRIVATE Threads::Threads)
