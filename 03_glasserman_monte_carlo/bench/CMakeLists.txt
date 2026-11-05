add_executable(
  glasserman_bench
  bench_ch01_ch02.cc bench_ch03_ch04.cc bench_ch05_ch06.cc bench_ch07_ch09.cc)
target_link_libraries(
  glasserman_bench
  PRIVATE glasserman_ch04_variance_reduction glasserman_ch05_quasi_monte_carlo
          glasserman_ch06_discretization glasserman_ch07_sensitivities
          glasserman_ch08_american_options glasserman_ch09_risk_management
          benchmark::benchmark_main)
