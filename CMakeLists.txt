cmake_minimum_required(VERSION 3.25)
project(cpp4q LANGUAGES CXX)

# One sub-project per book; each lives in its own top-level folder.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(CPP4Q_BUILD_TESTS "Build unit tests (GoogleTest)" ON)
option(CPP4Q_BUILD_BENCHMARKS "Build benchmarks (Google Benchmark)" ON)
option(CPP4Q_ENABLE_MODULES
       "Build C++20 named-module examples (needs Ninja + clang>=17/gcc>=14)" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Cpp4qDependencies)
include(Cpp4qWarnings)

if(CPP4Q_BUILD_TESTS)
  enable_testing()
endif()

add_subdirectory(01_hanson_modern_cpp_finance)
