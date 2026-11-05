# Prefer system packages, fall back to FetchContent so a bare machine works.
include(FetchContent)

if(CPP4Q_BUILD_TESTS)
  find_package(GTest CONFIG QUIET)
  if(NOT GTest_FOUND)
    find_package(GTest QUIET)
  endif()
  if(NOT GTest_FOUND)
    message(STATUS "GoogleTest not found, fetching")
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()
  include(GoogleTest)
endif()

if(CPP4Q_BUILD_BENCHMARKS)
  find_package(benchmark CONFIG QUIET)
  if(NOT benchmark_FOUND)
    message(STATUS "Google Benchmark not found, fetching")
    FetchContent_Declare(
      benchmark
      URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.tar.gz)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)
  endif()
endif()

# Boost is only needed for header-only Boost.Math (book 1, chapter 9).
find_package(Boost 1.75 QUIET)
if(Boost_FOUND)
  message(STATUS "Boost ${Boost_VERSION} found")
else()
  message(STATUS "Boost not found: Boost examples are skipped")
endif()

find_package(Threads REQUIRED)
