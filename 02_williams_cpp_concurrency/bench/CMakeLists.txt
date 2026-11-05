add_executable(
  williams_bench
  bench_ch02_ch03.cc bench_ch04.cc bench_ch05.cc bench_queues.cc
  bench_ch08.cc bench_ch09.cc bench_ch10.cc bench_appendix.cc)
target_link_libraries(
  williams_bench
  PRIVATE williams_ch02_managing_threads williams_ch03_sharing_data
          williams_ch04_synchronizing williams_ch05_memory_model
          williams_ch06_lock_based_ds williams_ch07_lock_free
          williams_ch08_designing_concurrent_code
          williams_ch09_advanced_thread_mgmt williams_ch10_parallel_algorithms
          williams_appendices benchmark::benchmark_main)
