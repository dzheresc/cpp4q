#include "ch11_testing_debugging/stress_harness.h"

namespace cpp4q::williams::ch11 {

void RunConcurrently(unsigned num_threads,
                     const std::function<void(unsigned)>& body) {
  if (num_threads == 0) return;
  std::latch start(static_cast<std::ptrdiff_t>(num_threads));
  std::mutex error_mutex;
  std::exception_ptr first_error;
  {
    std::vector<std::jthread> threads;
    threads.reserve(num_threads);
    for (unsigned i = 0; i < num_threads; ++i) {
      threads.emplace_back([&, i] {
        start.arrive_and_wait();
        try {
          body(i);
        } catch (...) {
          std::lock_guard lock(error_mutex);
          if (!first_error) first_error = std::current_exception();
        }
      });
    }
  }
  if (first_error) std::rethrow_exception(first_error);
}

int CountFailingRounds(int rounds, unsigned num_threads,
                       const std::function<void()>& setup,
                       const std::function<void(unsigned)>& body,
                       const std::function<bool()>& check) {
  int failures = 0;
  for (int r = 0; r < rounds; ++r) {
    if (setup) setup();
    RunConcurrently(num_threads, body);
    if (!check()) ++failures;
  }
  return failures;
}

}  // namespace cpp4q::williams::ch11
