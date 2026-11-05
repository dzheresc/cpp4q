# Book 5: Mark S. Joshi, "C++ Design Patterns and Derivatives Pricing",
# 2nd ed. (Cambridge University Press, 2008).
# One library + one demo executable per chapter and appendix.

set(JOSHI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# joshi_chapter(<name> [WHOLE_ARCHIVE] [SOURCES ...] [DEPS ...])
#   Creates library joshi_<name> (INTERFACE if no SOURCES) and, if
#   <name>/main.cc exists, an executable joshi_<name>_demo.
#   WHOLE_ARCHIVE: the sources hold self-registering objects that nothing
#   references (factory registrars), which a linker would drop from a
#   static library. The archive is then built as joshi_<name>_archive and
#   joshi_<name> is an interface target that links it whole.
function(joshi_chapter name)
  cmake_parse_arguments(ARG "WHOLE_ARCHIVE" "" "SOURCES;DEPS" ${ARGN})
  set(lib joshi_${name})
  if(ARG_SOURCES)
    list(TRANSFORM ARG_SOURCES PREPEND "${name}/")
    set(archive ${lib})
    if(ARG_WHOLE_ARCHIVE)
      set(archive ${lib}_archive)
    endif()
    add_library(${archive} STATIC ${ARG_SOURCES})
    target_include_directories(${archive} PUBLIC "${JOSHI_ROOT}")
    target_link_libraries(${archive} PUBLIC ${ARG_DEPS} PRIVATE cpp4q_warnings)
    if(ARG_WHOLE_ARCHIVE)
      add_library(${lib} INTERFACE)
      target_link_libraries(${lib}
                            INTERFACE "$<LINK_LIBRARY:WHOLE_ARCHIVE,${archive}>")
    endif()
  else()
    add_library(${lib} INTERFACE)
    target_include_directories(${lib} INTERFACE "${JOSHI_ROOT}")
    target_link_libraries(${lib} INTERFACE ${ARG_DEPS})
  endif()
  if(EXISTS "${JOSHI_ROOT}/${name}/main.cc")
    add_executable(${lib}_demo ${name}/main.cc)
    target_link_libraries(${lib}_demo PRIVATE ${lib} cpp4q_warnings)
  endif()
endfunction()

joshi_chapter(appendix_b_distributions SOURCES normals.cc)
joshi_chapter(appendix_a_black_scholes SOURCES black_scholes.cc
              DEPS joshi_appendix_b_distributions)
joshi_chapter(appendix_c_array SOURCES mj_array.cc)
joshi_chapter(ch01_simple_monte_carlo SOURCES simple_mc.cc
              DEPS joshi_appendix_a_black_scholes)
joshi_chapter(ch02_encapsulation SOURCES payoff1.cc
              DEPS joshi_ch01_simple_monte_carlo)
joshi_chapter(ch03_inheritance SOURCES payoff2.cc
              DEPS joshi_ch01_simple_monte_carlo)
joshi_chapter(ch04_virtual_constructor
              SOURCES payoff3.cc payoff_bridge.cc parameters.cc vanilla.cc
              DEPS joshi_ch01_simple_monte_carlo)
joshi_chapter(ch05_strategies_statistics SOURCES statistics.cc
              DEPS joshi_ch04_virtual_constructor)
joshi_chapter(ch06_random_numbers SOURCES random.cc
              DEPS joshi_ch05_strategies_statistics)
joshi_chapter(ch07_exotics_engine SOURCES path_dependent.cc exotic_engine.cc
              DEPS joshi_ch06_random_numbers)
joshi_chapter(ch08_trees SOURCES tree.cc DEPS joshi_ch04_virtual_constructor)
joshi_chapter(ch09_solvers_implied_vol SOURCES solvers.cc
              DEPS joshi_appendix_a_black_scholes)
joshi_chapter(ch10_factory WHOLE_ARCHIVE
              SOURCES payoff_factory.cc payoff_registration.cc
              DEPS joshi_ch05_strategies_statistics)
joshi_chapter(ch11_patterns_revisited SOURCES patterns.cc
              DEPS joshi_ch06_random_numbers)
joshi_chapter(ch12_situation_2007 SOURCES modern.cc
              DEPS joshi_ch04_virtual_constructor)
joshi_chapter(ch13_exceptions SOURCES exceptions.cc
              DEPS joshi_ch04_virtual_constructor)
joshi_chapter(ch14_template_factory WHOLE_ARCHIVE SOURCES registrations.cc
              DEPS joshi_ch11_patterns_revisited)
joshi_chapter(ch15_excel_interface SOURCES cells.cc excel_c_api.cc
              DEPS joshi_ch07_exotics_engine joshi_ch09_solvers_implied_vol
                   joshi_ch13_exceptions joshi_ch14_template_factory)
joshi_chapter(ch16_decoupling SOURCES mc_pricer.cc
              DEPS joshi_ch14_template_factory)

if(CPP4Q_BUILD_TESTS)
  add_subdirectory(tests)
endif()
if(CPP4Q_BUILD_BENCHMARKS)
  add_subdirectory(bench)
endif()

# Deterministic workloads for the valgrind scripts in profiling/.
add_executable(joshi_profile profiling/profile_driver.cc)
target_link_libraries(
  joshi_profile
  PRIVATE joshi_ch07_exotics_engine joshi_ch08_trees joshi_ch09_solvers_implied_vol
          joshi_ch12_situation_2007 joshi_ch14_template_factory
          joshi_ch16_decoupling joshi_appendix_c_array Threads::Threads
          cpp4q_warnings)
