# Book 2: Anthony Williams, "C++ Concurrency in Action", 2nd ed. (Manning, 2019).
# One library + one demo executable per chapter.

set(WILLIAMS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# williams_chapter(<name> [SOURCES ...] [DEPS ...])
#   Creates library williams_<name> (INTERFACE if no SOURCES) and, if
#   <name>/main.cc exists, an executable williams_<name>_demo.
function(williams_chapter name)
  cmake_parse_arguments(ARG "" "" "SOURCES;DEPS" ${ARGN})
  set(lib williams_${name})
  if(ARG_SOURCES)
    list(TRANSFORM ARG_SOURCES PREPEND "${name}/")
    add_library(${lib} STATIC ${ARG_SOURCES})
    target_include_directories(${lib} PUBLIC "${WILLIAMS_ROOT}")
    target_link_libraries(${lib} PUBLIC Threads::Threads ${ARG_DEPS}
                                 PRIVATE cpp4q_warnings)
  else()
    add_library(${lib} INTERFACE)
    target_include_directories(${lib} INTERFACE "${WILLIAMS_ROOT}")
    target_link_libraries(${lib} INTERFACE Threads::Threads ${ARG_DEPS})
  endif()
  if(EXISTS "${WILLIAMS_ROOT}/${name}/main.cc")
    add_executable(${lib}_demo ${name}/main.cc)
    target_link_libraries(${lib}_demo PRIVATE ${lib} cpp4q_warnings)
  endif()
endfunction()

williams_chapter(ch01_hello_concurrency SOURCES swap_pricing.cc)
williams_chapter(ch02_managing_threads SOURCES parallel_accumulate.cc)
williams_chapter(ch03_sharing_data SOURCES account.cc hierarchical_mutex.cc
                 quote_cache.cc)
williams_chapter(ch04_synchronizing SOURCES futures.cc)
williams_chapter(ch05_memory_model SOURCES atomic_stats.cc)
williams_chapter(ch06_lock_based_ds)
williams_chapter(ch07_lock_free)
williams_chapter(ch08_designing_concurrent_code SOURCES parallel_algorithms.cc
                 DEPS williams_ch02_managing_threads)
williams_chapter(ch09_advanced_thread_mgmt SOURCES thread_pool.cc)
williams_chapter(ch10_parallel_algorithms SOURCES portfolio_algos.cc)
if(TBB_FOUND)
  target_link_libraries(williams_ch10_parallel_algorithms PUBLIC TBB::tbb)
  target_compile_definitions(williams_ch10_parallel_algorithms
                             PUBLIC CPP4Q_HAVE_TBB=1)
endif()
williams_chapter(ch11_testing_debugging SOURCES stress_harness.cc)
williams_chapter(appendices SOURCES order_gateway.cc)

if(CPP4Q_BUILD_TESTS)
  add_subdirectory(tests)
endif()
if(CPP4Q_BUILD_BENCHMARKS)
  add_subdirectory(bench)
endif()

# Deterministic workloads for the valgrind scripts in profiling/.
add_executable(williams_profile profiling/profile_driver.cc)
target_link_libraries(
  williams_profile
  PRIVATE williams_ch02_managing_threads williams_ch03_sharing_data
          williams_ch04_synchronizing williams_ch05_memory_model
          williams_ch06_lock_based_ds williams_ch07_lock_free
          williams_ch08_designing_concurrent_code
          williams_ch09_advanced_thread_mgmt williams_ch10_parallel_algorithms
          williams_appendices cpp4q_warnings)
