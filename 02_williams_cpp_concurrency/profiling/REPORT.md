# Book 2 profiling report

Environment: 4-vCPU cloud container, GCC 13.3, glibc 2.39, oneTBB 2021.11.
Benchmarks are `-O3` (Release); valgrind 3.22 runs on `-O2 -g`
(RelWithDebInfo); ThreadSanitizer runs on a `-DCPP4Q_SANITIZE=thread`
build. The machine is shared and multi-threaded timings are noisy: between
the "before" run and the "after" run, the untouched serial controls
(`BM_FindSerial`, `BM_ScanSerial`, `BM_HistogramSerial`) got 30-60% slower.
So the "after" column is the median of 5 repetitions
(`results/bench_after_median.txt`), and only changes well beyond that noise
are claimed.

Reproduce:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
build/02_williams_cpp_concurrency/bench/williams_bench
02_williams_cpp_concurrency/profiling/run_tsan.sh             # builds build-tsan
cmake -S . -B build-prof -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCPP4Q_BUILD_BENCHMARKS=OFF && cmake --build build-prof
02_williams_cpp_concurrency/profiling/run_valgrind.sh build-prof
```

`profile_driver.cc` holds the fixed workloads that valgrind runs (22 of
them, one or more per chapter).

## Correctness tools: final state

| Tool | Target | Result |
| --- | --- | --- |
| ThreadSanitizer | 12 test binaries + 12 demos | 0 warnings, all pass (chapter 10 excluded: oneTBB is uninstrumented, so TSan can't see its synchronisation) |
| memcheck (`--leak-check=full --track-origins=yes`) | 12 test binaries + 12 demos | 0 errors, 0 bytes definitely/indirectly/possibly lost (one suppression for oneTBB worker-thread TLS, see below) |
| helgrind | 11 mutex/condvar workloads | 0 errors |
| drd | same 11 workloads | 0 errors |

helgrind and drd only understand pthread primitives. On the 10 workloads
synchronised by C++ atomics (SpinLock, SeqLock, SPSC/MPMC queues, the
Treiber stack, and libstdc++'s own futex-based `std::future` and
`std::barrier`) they report races on memory that is in fact ordered by
acquire/release: up to 200k reports on the SPSC ring. `run_valgrind.sh`
lists those as informational; TSan, which models atomics, is clean on all
of them.

## Correctness issues found, and fixes

| # | Found by | Issue | Fix |
| --- | --- | --- | --- |
| 1 | TSan | `LazyCurve` used `std::call_once`. When the curve builder throws (say the market-data feed is down), the next caller **hangs**: libstdc++ implements `call_once` with `pthread_once`, which doesn't support exceptions under TSan's interceptor, and doesn't on some targets natively either (GCC bug 66146). The plain build passed, so only TSan exposed it. | Double-checked locking done properly: an acquire load on the fast path, build under a mutex, release store. Retry-after-throw is now guaranteed and tested. |
| 2 | Code review | The book's two-lock queue (chapter 6) has a **lost-wakeup window**: `Push()` notifies without touching `head_mutex_`, but the waiter checks its predicate under `head_mutex_`, so the notify can land between that check and the wait. | `Push()` notifies while holding `head_mutex_`, and only when a consumer is waiting (see P4). |
| 3 | memcheck run | The first valgrind run **stalled for 20+ minutes** on the chapter 7 test (killed). Its producer/consumer loops spun on `TryPop()`/`TryPush()` without yielding; valgrind runs one thread at a time with long time slices, so each spinner burned its whole slice waiting for a thread that couldn't run. | Every spin loop in tests, demos and the driver yields on failure. The chapter 7 test under memcheck went from >23 min to **1.3 s**. The lesson carries over to production on oversubscribed hosts. |
| 4 | helgrind | `Transfer()` used `std::scoped_lock(a, b)`. It is deadlock-free, but its lock-and-try-lock dance takes the two mutexes in both orders, and helgrind reported 20k lock-order violations. | Lock the two accounts in address order. No back-off retries, and helgrind is clean. Timings are under P7. |
| 5 | helgrind, drd | 14k-40k "condition variable signalled but associated mutex not held" reports on the gateway and two-lock-queue workloads. Signalling after unlock is legal, but it hides real lost-wakeup bugs like #2 among the false positives. | Mailbox and two-lock queue notify under the lock. Mailbox throughput didn't change measurably (24.8 vs 23.5 ms per 2000 orders, median of 5). |
| 6 | memcheck | 912 bytes "possibly lost" in chapter 10: oneTBB never joins its worker threads, so their TLS blocks are still allocated at exit. | Not a leak. `profiling/valgrind.supp` suppresses exactly that stack (`_dl_allocate_tls` under `pthread_create` from `libtbb.so`). |
| 7 | Compiler | libstdc++ 13's PSTL fails to compile when given an rvalue policy (`std::execution::par_unseq_policy{}`), with errors deep inside `__is_vectorization_preferred`. | The chapter 10 functions pass the policy on as an lvalue, with a comment explaining why. |

## Performance: what the benchmarks and profiles found, and the fixes

| # | Benchmark | Finding | Fix | Before | After (median of 5) |
| --- | --- | --- | --- | --- | --- |
| P1 | `BM_PoolSubmit10k` (ch 9) | **14 µs per trivial task.** callgrind showed nothing hot; `strace -c` showed ~1.6 `futex` syscalls per task. Workers drained the queue faster than the producer filled it, went to sleep, and every `Submit()` paid a futex wake. Workers also took the shared and stealing mutexes even when those queues were empty, convoying with the producer. | Idle workers spin briefly (`pause`, 4096 polls) before parking. A `sleepers_` count with a seq_cst handshake means `Submit()` only signals when someone is asleep. Each queue keeps an atomic size, so workers skip empty queues without locking them. | 143 ms | **19.1 ms (7.5x)** |
| P2 | `BM_HistogramParallel/4` (ch 5) | **9x slower than the serial loop** (75.7 vs 8.4 ms). One `atomic_ref::fetch_add` per sample, and PnL clusters in a few central bins, so all four cores fought over the same cache lines. | Each thread counts into a private histogram, then publishes with one `atomic_ref` add per bin. | 75.7 ms | **3.25 ms (23x)**, now 3.5x faster than serial |
| P3 | `BM_HandOff_MutexCvQueue` (ch 4) | `condition_variable_any::notify_one` takes an internal mutex on every call, even with no waiters, so every `Push()` paid for it. | Count waiters under the queue mutex and notify only when someone is waiting. | 1P1C 118 ms, 2P2C 380 ms | **62.9 ms (1.9x), 129 ms (2.9x)** |
| P4 | `BM_HandOff_TwoLockQueue` (ch 6) | After fix #2, every push also took the head lock, so the two-lock queue lost its reason to exist: 2x slower than the one-lock queue. | Atomic waiter count with a seq_cst handshake; `Push()` takes the head lock only when a consumer sleeps. | 1P1C 250 ms, 2P2C 211 ms | **133 ms (1.9x), 98.2 ms (2.1x)** |
| P5 | `BM_LockedIncrement<SpinLock>/4` (ch 5) | The spin lock was 2.2x slower than `std::mutex` with 4 threads. | Added a `pause` hint in the spin loop. Exponential backoff was tried and didn't help, so it was removed. | 156 ns | 136 ns (in the noise). Still 1.8x slower than `std::mutex` (74 ns). On a shared 4-vCPU VM a futex-backed mutex beats spinning. The spin lock stays as a chapter 5 example and wins only uncontended (17.9 vs 21.3 ns). |
| P6 | `BM_LockstepMeanSpot/4` (ch 4) | The barrier completion step summed all 100k paths serially while every worker waited (Amdahl). | Each worker sums its own slice into its own padded slot; the completion step adds 4 numbers. | 66.2 ms | 76.3 ms; the serial controls moved more than that, so **no measurable change**. Kept because it removes a serial pass that grows with the path count, and because helgrind's reports on this workload fell from 1.8M to 1.2k. |
| P7 | `BM_TransferContended` (ch 3) | Effect of fix #4 (address-ordered locking). | n/a | 1 thread 53.1 ns; 4 threads 146 ns | 1 thread **43.8 ns (1.2x)**; 2-4 threads 168-184 ns (**about 20% slower**, borderline noise). Kept for the helgrind-clean, retry-free lock order; `scoped_lock(a, b)` is the right call when no global order is available. |

## Measurements that make the chapters' points (no change needed)

* **False sharing (ch 8):** with 4 threads each incrementing only its own
  counter, packed counters take 73-101 ms; padding each to its own cache
  line takes 11.7-12.5 ms, **6-8x** faster. Packed gets *slower* as threads
  are added; padded scales 3x. cachegrind can't show this, because it
  simulates one core's caches and so has no coherence traffic.
* **Queue hand-off, 1 producer -> 1 consumer, 262k ints (ch 4/6/7):** mutex+condvar
  63 ms, two-lock 133 ms, lock-free MPMC 12.1 ms, SPSC ring **3.1 ms**
  (85M items/s, 20x the mutex queue). The SPSC ring's cached indices mean
  most operations touch no shared cache line.
* **Readers (ch 3 vs ch 5):** `shared_mutex` quote-cache reads go from 43
  ns (1 thread) to 74-100 ns (4 threads): even shared locking writes the
  reader count, and that cache line bounces. SeqLock reads scale the other
  way, from 7.5 ns to **2.0 ns** per read across 4 threads, because readers
  never write. For hot read-mostly market data, a seqlock is the right tool.
* **Lock-free isn't automatically faster (ch 7):** the Treiber stack costs
  53-64 ns per push/pop uncontended versus 38-48 ns for a mutex-guarded
  vector, because every push allocates a node. Under 4-thread contention
  both are about 150-220 ns. MPMC with 2P2C (30 ms) is slower than 1P1C
  (12 ms): the CAS on the shared counters becomes the bottleneck.
* **Thread costs (ch 2/4):** spawn+join is 40-48 µs; a round trip through
  the long-lived `PricingService` thread is 27-31 µs; `std::async` (a new
  thread each time) is 40-48 µs. In this sandbox each futex wake is
  expensive, which is also why P1 mattered so much.
* **Parallel algorithms (ch 10, oneTBB):** `transform_reduce` PV of 1M bonds
  goes from 18.0 ms (`seq`) to 4.8 ms (`par`, 3.7x); `par_unseq` gains
  nothing extra because the per-bond loop has a data-dependent trip count
  and doesn't vectorise. `sort` goes from 131 to 32 ms (4.1x).
* **Hand-written vs library (ch 4/8/9):** the `std::async` quicksort is
  2.3x faster than `std::sort` on 1M doubles, and the pool quicksort 3x.
  `ParallelInclusiveScan` (3 passes) is 3.1x faster than
  `std::inclusive_scan`. `ParallelFind` returns the *first* match like
  `std::find_if` and is 1.5x faster at 90% depth. `ScenarioVar` gains only
  1.5x at 4 threads, because at 20k x 50 the `nth_element` and thread
  start-up are a large share of the total.

## Not changed, on purpose

* The Treiber stack's "threads in pop" reclamation can let the pending
  list grow without bound under constant contention. Hazard pointers or
  split reference counts (also in chapter 7) bound it at a higher
  per-operation cost. The trade-off is noted in the header.
* `HistoricalVarAsync` (ch 4) detaches its thread on purpose, to show
  promise/future as the only link back. memcheck and TSan are clean because
  callers `.get()` before exit; in production code, prefer a pool or
  `std::async`.
* Mailbox throughput (about 12 µs per order across three threads) is
  bounded by a futex wake per message. Batching or a spin-then-park
  receive like P1 would help, but the appendix is about the actor
  structure, not speed.
