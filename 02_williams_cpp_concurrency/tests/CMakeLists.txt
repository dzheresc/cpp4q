set(WILLIAMS_TESTS
    ch01_hello_concurrency ch02_managing_threads ch03_sharing_data
    ch04_synchronizing ch05_memory_model ch06_lock_based_ds ch07_lock_free
    ch08_designing_concurrent_code ch09_advanced_thread_mgmt
    ch10_parallel_algorithms ch11_testing_debugging appendices)

foreach(ch IN LISTS WILLIAMS_TESTS)
  add_executable(williams_${ch}_test ${ch}_test.cc)
  target_link_libraries(williams_${ch}_test
                        PRIVATE williams_${ch} GTest::gtest_main cpp4q_warnings)
  gtest_discover_tests(williams_${ch}_test DISCOVERY_TIMEOUT 30)
endforeach()
