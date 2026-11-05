#include "ch02_random_numbers/parallel_mc.h"

namespace cpp4q::glasserman::ch02 {

unsigned DefaultThreadCount() {
  const unsigned hw = std::thread::hardware_concurrency();
  return hw ? hw : 2;
}

}  // namespace cpp4q::glasserman::ch02
