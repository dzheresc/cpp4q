add_library(cpp4q_warnings INTERFACE)
target_compile_options(
  cpp4q_warnings
  INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wconversion;-Wsign-conversion;-Wnon-virtual-dtor;-Wold-style-cast;-Woverloaded-virtual>
    $<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->)
