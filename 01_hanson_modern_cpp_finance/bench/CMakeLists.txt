set(HANSON_BENCH_SOURCES
    bench_ch01_ch02.cc bench_ch03.cc bench_ch04.cc bench_ch05.cc
    bench_ch06.cc bench_ch07.cc bench_ch08.cc bench_ch10.cc)
set(HANSON_BENCH_LIBS
    hanson_ch01_overview hanson_ch02_classes hanson_ch03_polymorphism
    hanson_ch04_stl_containers hanson_ch05_algorithms_ranges
    hanson_ch06_random_concurrency hanson_ch07_dates_fixed_income
    hanson_ch08_linear_algebra hanson_ch10_modules_concepts)
if(Boost_FOUND)
  list(APPEND HANSON_BENCH_SOURCES bench_ch09.cc)
  list(APPEND HANSON_BENCH_LIBS hanson_ch09_boost)
endif()

add_executable(hanson_bench ${HANSON_BENCH_SOURCES})
target_link_libraries(hanson_bench PRIVATE ${HANSON_BENCH_LIBS}
                                           benchmark::benchmark_main)
