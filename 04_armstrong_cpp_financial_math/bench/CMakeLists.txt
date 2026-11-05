add_executable(
  armstrong_bench
  bench_basics.cc bench_pricing.cc bench_matrix.cc bench_callables.cc)
target_link_libraries(
  armstrong_bench
  PRIVATE armstrong_ch02_data_types armstrong_ch13_portfolio
          armstrong_ch14_delta_hedging armstrong_ch16_matrix
          armstrong_ch18_stl armstrong_ch19_function_objects
          armstrong_ch20_threads armstrong_ch21_next_steps
          armstrong_appendix_a_risk_neutral benchmark::benchmark_main)
