# Book 1 profiling report

Environment: 4-vCPU cloud container, GCC 13.3 (`-O3`, Release) for
benchmarks, `-O2 -g` (RelWithDebInfo) under valgrind 3.22. Clang 18 builds
and passes the same tests. Shared hardware, so expect roughly 10-15%
run-to-run noise on timings (for example, `BM_MultiplyNaive/512` didn't
change between runs but measured 347 ms and then 291 ms).

Reproduce:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
build/01_hanson_modern_cpp_finance/bench/hanson_bench
cmake -S . -B build-prof -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCPP4Q_BUILD_BENCHMARKS=OFF && cmake --build build-prof
01_hanson_modern_cpp_finance/profiling/run_valgrind.sh build-prof
```

The full benchmark output is in `results/bench_before.txt` and
`results/bench_after.txt`. `profile_driver.cc` holds the fixed workloads
that valgrind runs (`stats`, `mc`, `mc_parallel`, `matrix`, `tree`, `bonds`,
`orderbook`).

## Correctness tools

| Tool | Target | Result |
| --- | --- | --- |
| memcheck (`--leak-check=full --track-origins=yes`) | all 10 test binaries + 11 demos | 0 errors, 0 bytes leaked |
| helgrind | `mc_parallel` (4 `std::jthread` workers) | 0 errors |
| drd | `mc_parallel` | 0 errors |

## What callgrind / cachegrind / massif found, and the fixes

| # | Workload | Finding | Fix | Before | After |
| --- | --- | --- | --- | --- | --- |
| 1 | `tree` | callgrind: `__ieee754_pow` was **81%** of Ir. The American tree recomputed `S·u^j·d^(n-j)` with two `std::pow` calls per node. | Walk spots multiplicatively (`node *= u²`), fold the discount into `q_up`/`q_down`. | 151.9 M Ir; American/2048 57.6 ms | 10.1 M Ir; **2.97 ms (19x)** |
| 2 | `matrix` | callgrind: `MultiplyNaive` was 88% of Ir. cachegrind: **43.5% D1 miss rate** (34.5 M misses), because the inner loop strides down a column of B. | `Multiply` is now i-k-j with the k loop tiled at 64. Both inner rows are unit-stride and auto-vectorise. | D1 misses 34.5 M; 512³ 367 ms (0.73 GFLOP/s) | D1 misses 4.9 M (4.4%); **62.9 ms (4.3 GFLOP/s, 5.8x)** |
| 3 | `stats` | callgrind: `MovingAverage` was **65%** of Ir. The ranges pipeline re-reduced every window, which is O(n·w). | Sliding sum, re-summed exactly every `w` outputs so rounding can't drift. The ranges version stays as `MovingAverageRanges`, the reference in tests and benchmarks. | 169 M Ir; w=2000 59.8 ms | 57.6 M Ir; **0.38 ms (158x)** |
| 4 | `stats` | massif: an n-sized `peaks` buffer from `MaxDrawdown`'s `inclusive_scan` showed up in the peak snapshot. | Single pass with a running peak and no allocation. | 3.21 ms @ 1M | **1.72 ms (1.9x)** |
| 5 | `mc` | callgrind: `exp` was the top self-cost symbol (36%). Antithetic paths called `exp` twice per step. | `e^(m−vz) = e^(2m) / e^(m+vz)`, so the mirror path costs one division instead of a second `exp`. | 366 M Ir; Asian 45.7 ms | 283 M Ir; **36.5 ms (1.25x)**; European 4.61 → 3.80 ms |
| 6 | `mc_parallel` | Review while reading the drd output: each `jthread` called `partial[i].Add()` once per path, and neighbouring `SampleStats` share a cache line (false sharing). | Accumulate in a thread-local object and publish once at the end. | 4 threads 70.0 ms (3.4x vs 1 thread) | **45.6 ms (3.9x vs 1 thread)**, together with fix 5 |
| 7 | `bonds` | callgrind: `DaysBetween`/`YearFraction` (≈50%) plus `pow` (≈20%). Every Newton iteration recomputed the day-count fractions and called `pow` once per cash flow. | Compute `(t, amount)` once per solve. Use `exp(−f·t·log(1+y/f))`, which needs one `log` per iteration. | 31.2 M Ir; YTM 7.2 µs | 16.9 M Ir; **2.0 µs (3.6x)**; `Risk` 1.6 → 0.95 µs |

Things I tried and reverted:

* **HistoricalVar with a bounded heap** (`partial_sort_copy` into k+1
  slots, O(k) memory). It ran 1.6x *slower* at n = 262k, and the massif peak
  didn't move, because the peak is set by the caller's own n-sized vectors.
  The code keeps the copy + `nth_element` version, with a comment saying why.

Not changed, on purpose:

* `orderbook`: the RNG in the driver costs 42% of Ir, and `std::map` work
  in `Update` costs about 33%. 49 ns per update is fine for a teaching
  example. A flat sorted vector or a price-indexed array would be the next
  step.
* The Monte Carlo pricers still spend most of their time in
  `std::normal_distribution` (Marsaglia polar: a `log`, a `sqrt` and
  rejections) and in `std::mt19937_64`. Swapping in a Ziggurat or
  inverse-CDF sampler would change the random stream. Glasserman (book 3)
  covers that properly, so it is left for that book.
* `MultiplyValarray` (Appendix E) builds two temporaries per output element
  and runs at about 1 GFLOP/s. It is there to show slices, not speed.

## Numerical issue found by the tests

`RollingWindow` started out with sum and sum-of-squares, which is the
textbook O(1) approach. A test with prices near 1e6 and noise of 1e-3 lost
every significant digit of the variance. I moved it to a rolling Welford
update, but a 10k-step run still drifted by 2e-6 relative. The window now
also recomputes its moments exactly once per full turn of the ring. The
amortised cost is still O(1), and the error stays bounded.
