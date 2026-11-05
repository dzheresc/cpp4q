# Book 2: C++ Concurrency in Action, 2nd ed. (Anthony Williams, Manning 2019)

This is original C++20 code for each chapter, in Google C++ style, built with
CMake. It does not reproduce the book's listings. The examples are trading
and risk flavoured: swap legs, accounts, quote caches, order queues, top of
book, position books, VaR, an order gateway. Each chapter is a small
library, a `main.cc` demo and a GoogleTest suite. Benchmarks, valgrind and
ThreadSanitizer cover the whole book.

| Chapter | Folder | What it shows |
| --- | --- | --- |
| 1. Hello, world of concurrency | `ch01_hello_concurrency/` | `std::thread` + `join()`: the two legs of a swap valued concurrently |
| 2. Managing threads | `ch02_managing_threads/` | `ScopedThread`, a move-only `JoiningThread`, ownership transfer, run-time thread count, `ParallelAccumulate` over portfolio PnL |
| 3. Sharing data between threads | `ch03_sharing_data/` | `std::scoped_lock` for deadlock-free account transfers, a run-time-checked `HierarchicalMutex`, a `std::shared_mutex` quote cache, lazy curve construction |
| 4. Synchronizing concurrent operations | `ch04_synchronizing/` | condition-variable queue with `Close()` and `stop_token` cancellation, `packaged_task` pricing service, `promise` for async VaR, `std::async` quicksort, `std::latch` + `std::barrier` lock-step GBM simulation |
| 5. The memory model and atomics | `ch05_memory_model/` | TTAS `SpinLock` with C++20 `atomic::wait`, a data-race-free `SeqLock` for top of book, relaxed counters, `std::atomic_ref` histogram, release/acquire `PublishOnce` |
| 6. Lock-based data structures | `ch06_lock_based_ds/` | two-mutex queue with a dummy node, bucketed lookup table with per-bucket `shared_mutex` (position book), hand-over-hand locked list |
| 7. Lock-free data structures | `ch07_lock_free/` | SPSC ring buffer with cached indices, Vyukov bounded MPMC queue, Treiber stack with threads-in-pop reclamation |
| 8. Designing concurrent code | `ch08_designing_concurrent_code/` | exception-safe `ParallelForEach`, `ParallelFind` with early exit and first-match semantics, three-phase `ParallelInclusiveScan`, false sharing (packed vs padded counters), parallel scenario VaR |
| 9. Advanced thread management | `ch09_advanced_thread_mgmt/` | work-stealing `ThreadPool` (per-thread deques, `WaitFor` that helps), recursive quicksort on the pool, `jthread`/`stop_token` cancellable repricer |
| 10. Parallel algorithms | `ch10_parallel_algorithms/` | `transform_reduce`, `transform`, `sort`, `count_if`, `any_of` under `seq` / `par` / `par_unseq` on a bond book (oneTBB backend) |
| 11. Testing and debugging | `ch11_testing_debugging/` | a latch-started stress harness that rethrows worker exceptions; a pre-trade limit with a check-then-act race (no data race, still wrong) and its CAS fix |
| Appendix C: message passing | `appendices/` | `std::variant` mailboxes + `std::visit` overload sets; an order gateway and simulated exchange as actors (the book's ATM, finance edition) |

## Build

From the repository root:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build                                    # 148 tests in book 2
build/02_williams_cpp_concurrency/williams_ch07_lock_free_demo
build/02_williams_cpp_concurrency/bench/williams_bench    # Google Benchmark
```

oneTBB (`libtbb-dev`) is optional. libstdc++ uses it as the backend for
`std::execution::par`; without it chapter 10 still builds and its parallel
policies run serially.

ThreadSanitizer build of the whole repository:

```sh
cmake -S . -B build-tsan -G Ninja -DCPP4Q_SANITIZE=thread -DCPP4Q_BUILD_BENCHMARKS=OFF
```

## Profiling

`profiling/run_valgrind.sh` runs memcheck, helgrind, drd, callgrind,
cachegrind and massif over `profiling/profile_driver.cc`'s workloads;
`profiling/run_tsan.sh` runs every test and demo under ThreadSanitizer.
[`profiling/REPORT.md`](profiling/REPORT.md) lists what they found, the
fixes, and before/after benchmark numbers.
