# Book 1: Daniel Hanson, "Learning Modern C++ for Finance" (O'Reilly, 2024).
# One static library + one example executable per chapter.

set(HANSON_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# hanson_chapter(<name> [SOURCES ...] [DEPS ...])
#   Creates library hanson_<name> (INTERFACE if no SOURCES) and, if
#   <name>/main.cc exists, an executable hanson_<name>_demo.
function(hanson_chapter name)
  cmake_parse_arguments(ARG "" "" "SOURCES;DEPS" ${ARGN})
  set(lib hanson_${name})
  if(ARG_SOURCES)
    list(TRANSFORM ARG_SOURCES PREPEND "${name}/")
    add_library(${lib} STATIC ${ARG_SOURCES})
    target_include_directories(${lib} PUBLIC "${HANSON_ROOT}")
    target_link_libraries(${lib} PUBLIC ${ARG_DEPS} PRIVATE cpp4q_warnings)
  else()
    add_library(${lib} INTERFACE)
    target_include_directories(${lib} INTERFACE "${HANSON_ROOT}")
    target_link_libraries(${lib} INTERFACE ${ARG_DEPS})
  endif()
  if(EXISTS "${HANSON_ROOT}/${name}/main.cc")
    add_executable(${lib}_demo ${name}/main.cc)
    target_link_libraries(${lib}_demo PRIVATE ${lib} cpp4q_warnings)
  endif()
endfunction()

hanson_chapter(ch01_overview SOURCES black_scholes.cc)
hanson_chapter(ch02_classes SOURCES european_option.cc price_buffer.cc
               DEPS hanson_ch01_overview)
hanson_chapter(ch03_polymorphism SOURCES payoff.cc yield_curve.cc option.cc)
hanson_chapter(ch04_stl_containers SOURCES order_book.cc portfolio.cc
               rolling_window.cc)
hanson_chapter(ch05_algorithms_ranges SOURCES return_stats.cc)
hanson_chapter(ch06_random_concurrency SOURCES monte_carlo.cc
               DEPS Threads::Threads)
hanson_chapter(ch07_dates_fixed_income SOURCES calendar.cc day_count.cc
               schedule.cc bond.cc)
hanson_chapter(ch08_linear_algebra SOURCES matrix.cc decompositions.cc
               valarray_ops.cc)
if(Boost_FOUND)
  hanson_chapter(ch09_boost SOURCES boost_pricing.cc DEPS Boost::headers
                 hanson_ch01_overview)
endif()
hanson_chapter(ch10_modules_concepts DEPS hanson_ch03_polymorphism)

add_executable(hanson_appendices_demo appendices/main.cc)
target_link_libraries(hanson_appendices_demo PRIVATE hanson_ch03_polymorphism
                                                     cpp4q_warnings)

if(CPP4Q_ENABLE_MODULES)
  add_library(hanson_ch10_module)
  target_sources(hanson_ch10_module PUBLIC FILE_SET CXX_MODULES FILES
                 ch10_modules_concepts/pricing.cppm)
  target_compile_features(hanson_ch10_module PUBLIC cxx_std_20)
  add_executable(hanson_ch10_module_demo ch10_modules_concepts/module_main.cc)
  target_link_libraries(hanson_ch10_module_demo PRIVATE hanson_ch10_module)
  # The project's minimum CMake predates CMP0155, so opt in to dependency
  # scanning explicitly for the two module targets.
  set_target_properties(hanson_ch10_module hanson_ch10_module_demo
                        PROPERTIES CXX_SCAN_FOR_MODULES ON)
endif()

if(CPP4Q_BUILD_TESTS)
  add_subdirectory(tests)
endif()
if(CPP4Q_BUILD_BENCHMARKS)
  add_subdirectory(bench)
endif()

# Deterministic workloads for the valgrind scripts in profiling/.
add_executable(hanson_profile profiling/profile_driver.cc)
target_link_libraries(
  hanson_profile
  PRIVATE hanson_ch03_polymorphism hanson_ch04_stl_containers
          hanson_ch05_algorithms_ranges hanson_ch06_random_concurrency
          hanson_ch07_dates_fixed_income hanson_ch08_linear_algebra
          hanson_ch10_modules_concepts)
