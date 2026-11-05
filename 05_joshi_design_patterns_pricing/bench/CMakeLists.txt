add_executable(joshi_bench bench_monte_carlo.cc bench_engines.cc bench_patterns.cc)
target_link_libraries(
  joshi_bench
  PRIVATE joshi_ch01_simple_monte_carlo joshi_ch07_exotics_engine
          joshi_ch08_trees joshi_ch09_solvers_implied_vol
          joshi_ch12_situation_2007 joshi_ch14_template_factory
          joshi_ch16_decoupling joshi_appendix_c_array
          benchmark::benchmark_main)
