# Book 3: Paul Glasserman, "Monte Carlo Methods in Financial Engineering"
# (Springer, 2003). One library + one demo executable per chapter.

set(GLASSERMAN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# glasserman_chapter(<name> [SOURCES ...] [DEPS ...])
#   Creates library glasserman_<name> (INTERFACE if no SOURCES) and, if
#   <name>/main.cc exists, an executable glasserman_<name>_demo.
function(glasserman_chapter name)
  cmake_parse_arguments(ARG "" "" "SOURCES;DEPS" ${ARGN})
  set(lib glasserman_${name})
  if(ARG_SOURCES)
    list(TRANSFORM ARG_SOURCES PREPEND "${name}/")
    add_library(${lib} STATIC ${ARG_SOURCES})
    target_include_directories(${lib} PUBLIC "${GLASSERMAN_ROOT}")
    target_link_libraries(${lib} PUBLIC ${ARG_DEPS} PRIVATE cpp4q_warnings)
  else()
    add_library(${lib} INTERFACE)
    target_include_directories(${lib} INTERFACE "${GLASSERMAN_ROOT}")
    target_link_libraries(${lib} INTERFACE ${ARG_DEPS})
  endif()
  if(EXISTS "${GLASSERMAN_ROOT}/${name}/main.cc")
    add_executable(${lib}_demo ${name}/main.cc)
    target_link_libraries(${lib}_demo PRIVATE ${lib} cpp4q_warnings)
  endif()
endfunction()

glasserman_chapter(ch01_foundations SOURCES black_scholes.cc monte_carlo.cc)
glasserman_chapter(ch02_random_numbers
                   SOURCES rng.cc normal.cc distributions.cc parallel_mc.cc
                   DEPS glasserman_ch01_foundations Threads::Threads)
glasserman_chapter(ch03_sample_paths
                   SOURCES brownian.cc gbm.cc short_rate.cc jump_diffusion.cc
                   DEPS glasserman_ch02_random_numbers)
glasserman_chapter(ch04_variance_reduction SOURCES variance_reduction.cc
                   DEPS glasserman_ch03_sample_paths)
glasserman_chapter(ch05_quasi_monte_carlo SOURCES low_discrepancy.cc qmc_pricing.cc
                   DEPS glasserman_ch03_sample_paths)
glasserman_chapter(ch06_discretization SOURCES schemes.cc
                   DEPS glasserman_ch03_sample_paths)
glasserman_chapter(ch07_sensitivities SOURCES greeks.cc
                   DEPS glasserman_ch03_sample_paths)
glasserman_chapter(ch08_american_options SOURCES lattice.cc lsm.cc random_tree.cc
                   DEPS glasserman_ch03_sample_paths)
glasserman_chapter(ch09_risk_management SOURCES portfolio_var.cc credit.cc
                   DEPS glasserman_ch03_sample_paths)

if(CPP4Q_BUILD_TESTS)
  add_subdirectory(tests)
endif()
if(CPP4Q_BUILD_BENCHMARKS)
  add_subdirectory(bench)
endif()

# Deterministic workloads for the valgrind scripts in profiling/.
add_executable(glasserman_profile profiling/profile_driver.cc)
target_link_libraries(
  glasserman_profile
  PRIVATE glasserman_ch04_variance_reduction glasserman_ch05_quasi_monte_carlo
          glasserman_ch06_discretization glasserman_ch07_sensitivities
          glasserman_ch08_american_options glasserman_ch09_risk_management
          cpp4q_warnings)
