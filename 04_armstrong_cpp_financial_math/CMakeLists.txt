# Book 4: John Armstrong, "C++ for Financial Mathematics" (CRC Press, 2017).
# One library + one demo executable per chapter.

set(ARMSTRONG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# armstrong_chapter(<name> [SOURCES ...] [DEPS ...])
#   Creates library armstrong_<name> (INTERFACE if no SOURCES) and, if
#   <name>/main.cc exists, an executable armstrong_<name>_demo.
function(armstrong_chapter name)
  cmake_parse_arguments(ARG "" "" "SOURCES;DEPS" ${ARGN})
  set(lib armstrong_${name})
  if(ARG_SOURCES)
    list(TRANSFORM ARG_SOURCES PREPEND "${name}/")
    add_library(${lib} STATIC ${ARG_SOURCES})
    target_include_directories(${lib} PUBLIC "${ARMSTRONG_ROOT}")
    target_link_libraries(${lib} PUBLIC ${ARG_DEPS} PRIVATE cpp4q_warnings)
  else()
    add_library(${lib} INTERFACE)
    target_include_directories(${lib} INTERFACE "${ARMSTRONG_ROOT}")
    target_link_libraries(${lib} INTERFACE ${ARG_DEPS})
  endif()
  if(EXISTS "${ARMSTRONG_ROOT}/${name}/main.cc")
    add_executable(${lib}_demo ${name}/main.cc)
    target_link_libraries(${lib}_demo PRIVATE ${lib} cpp4q_warnings)
  endif()
endfunction()

armstrong_chapter(ch01_getting_started SOURCES interest.cc)
armstrong_chapter(ch02_data_types SOURCES numerics.cc)
armstrong_chapter(ch03_functions SOURCES matlib.cc)
armstrong_chapter(ch04_flow_of_control SOURCES control.cc
                  DEPS armstrong_ch03_functions)
armstrong_chapter(ch05_multiple_files SOURCES vectors.cc
                  DEPS armstrong_ch03_functions)
armstrong_chapter(ch06_unit_testing SOURCES testing.cc
                  DEPS armstrong_ch03_functions)
armstrong_chapter(ch07_using_classes SOURCES charts.cc)
armstrong_chapter(ch08_user_defined_types SOURCES options.cc
                  DEPS armstrong_ch03_functions)
armstrong_chapter(ch09_monte_carlo SOURCES monte_carlo.cc
                  DEPS armstrong_ch08_user_defined_types armstrong_ch05_multiple_files)
armstrong_chapter(ch10_interfaces SOURCES interfaces.cc
                  DEPS armstrong_ch09_monte_carlo)
armstrong_chapter(ch11_arrays_strings_pointers SOURCES arrays.cc)
armstrong_chapter(ch12_sophisticated_classes SOURCES path_dependent.cc
                  DEPS armstrong_ch10_interfaces)
armstrong_chapter(ch13_portfolio SOURCES portfolio.cc
                  DEPS armstrong_ch10_interfaces)
armstrong_chapter(ch14_delta_hedging SOURCES hedging.cc
                  DEPS armstrong_ch10_interfaces)
armstrong_chapter(ch15_debugging_tools SOURCES debug.cc)
armstrong_chapter(ch16_matrix SOURCES matrix.cc multi_stock.cc
                  DEPS armstrong_ch09_monte_carlo)
armstrong_chapter(ch17_templates DEPS armstrong_ch03_functions)
armstrong_chapter(ch18_stl SOURCES stl.cc)
armstrong_chapter(ch19_function_objects SOURCES solvers.cc
                  DEPS armstrong_ch10_interfaces armstrong_ch04_flow_of_control
                       armstrong_ch17_templates)
armstrong_chapter(ch20_threads SOURCES parallel_pricer.cc
                  DEPS armstrong_ch12_sophisticated_classes Threads::Threads)
armstrong_chapter(ch21_next_steps SOURCES pde.cc
                  DEPS armstrong_ch03_functions)
armstrong_chapter(appendix_a_risk_neutral SOURCES binomial.cc
                  DEPS armstrong_ch03_functions)

if(CPP4Q_BUILD_TESTS)
  add_subdirectory(tests)
endif()
if(CPP4Q_BUILD_BENCHMARKS)
  add_subdirectory(bench)
endif()

# Deterministic workloads for the valgrind scripts in profiling/.
add_executable(armstrong_profile profiling/profile_driver.cc)
target_link_libraries(
  armstrong_profile
  PRIVATE armstrong_ch13_portfolio armstrong_ch14_delta_hedging
          armstrong_ch16_matrix armstrong_ch18_stl armstrong_ch19_function_objects
          armstrong_ch20_threads armstrong_ch21_next_steps
          armstrong_appendix_a_risk_neutral cpp4q_warnings)
