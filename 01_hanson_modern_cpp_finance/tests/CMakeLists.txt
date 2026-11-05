set(HANSON_TESTS
    ch01_overview ch02_classes ch03_polymorphism ch04_stl_containers
    ch05_algorithms_ranges ch06_random_concurrency ch07_dates_fixed_income
    ch08_linear_algebra ch10_modules_concepts)
if(Boost_FOUND)
  list(APPEND HANSON_TESTS ch09_boost)
endif()

foreach(ch IN LISTS HANSON_TESTS)
  add_executable(hanson_${ch}_test ${ch}_test.cc)
  target_link_libraries(hanson_${ch}_test PRIVATE hanson_${ch} GTest::gtest_main
                                                  cpp4q_warnings)
  gtest_discover_tests(hanson_${ch}_test)
endforeach()
target_link_libraries(hanson_ch10_modules_concepts_test
                      PRIVATE hanson_ch03_polymorphism)
