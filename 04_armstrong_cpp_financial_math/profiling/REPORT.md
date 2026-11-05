# Book 4 profiling report

Environment: 4-vCPU shared cloud container, GCC 13.3, glibc 2.39,
valgrind 3.22. Benchmarks are `-O3` (Release, median of 3 runs before and
5 after); valgrind runs on `-O2 -g` (RelWithDebInfo); the sanitizers run on
RelWithDebInfo builds with `-DCPP4Q_SANITIZE=...`.

Wall-clock timings on this machine are noisy: in the full "after" run,
benchmarks whose code did not change at all (`BM_NormInv`,
`BM_IntegrateFunctionPointer`, `BM_FixedMatrix4x4`) came out 20-30% slower
than in the "before" run. Only changes well beyond that are claimed below,
each one is backed by callgrind instruction counts (deterministic), and the
one borderline case (path-dependent Monte Carlo) was settled by running the
old and new binaries alternately in the same minute.

Reproduce:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
build/04_armstrong_cpp_financial_math/bench/armstrong_bench --benchmark_repetitions=5
cmake -S . -B build-prof -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCPP4Q_BUILD_BENCHMARKS=OFF && cmake --build build-prof
04_armstrong_cpp_financial_math/profiling/run_valgrind.sh build-prof
04_armstrong_cpp_financial_math/profiling/run_sanitizers.sh
```

`profile_driver.cc` holds the 16 fixed workloads that valgrind runs.
Raw outputs: `results/bench_{before,after}.txt`,
`results/valgrind_summary_{before,after}.txt`,
`results/sanitizers_summary.txt`.

## Correctness tools: final state

| Tool | Target | Result |
| --- | --- | --- |
| memcheck (`--leak-check=full --track-origins=yes`) | 22 test binaries + 22 demos | 0 errors, 0 bytes lost, no suppressions |
| helgrind | chapter 20: atomic block counter + `jthread`, `std::async`, mutex | 0 errors |
| drd | same | 0 errors |
| AddressSanitizer + UndefinedBehaviorSanitizer | 22 test binaries + 22 demos | 0 reports, all pass |
| ThreadSanitizer | 22 test binaries + 22 demos | 0 reports, all pass |

The parallel pricer is also bit-identical for 1, 2, 3, 4, 16 and 32
threads (tested), which is a stronger property than race freedom.

## Correctness issues found, and fixes

| # | Found by | Issue | Fix |
| --- | --- | --- | --- |
| 1 | Unit test | The Crank-Nicolson PDE price was 2.5e-3 off Black-Scholes at the money on the default 400 x 400 grid, and gamma 0.75% off, with errors that jumped around as the grid changed: they depended on where the strike and the spot fell relative to the nodes (quadratic interpolation of gamma is only first-order accurate between nodes). | The grid is stretched so the spot is a node, and the initial condition is the cell-averaged payoff. Default-grid error fell to **1.4e-4 (18x)**, gamma error to 1.8e-6; the tests now hold the PDE to 3e-4 on the default grid and 5e-5 on a fine one, plus a second-order convergence check. |
| 2 | memcheck | `MachineEpsilon<long double>()` returned 2.2e-16 instead of 1.08e-19 under valgrind, failing the test. Not a bug in the code: valgrind executes x87 80-bit arithmetic at 64-bit precision (a documented limitation), and the function measures the arithmetic actually executed. | The test checks against what the machine really does (`1 + LDBL_EPSILON != 1`), so it passes natively and under valgrind, and the chapter's point (epsilon is a property of the arithmetic, not of the type name) is now tested. |
| 3 | Script review | `run_valgrind.sh` ran memcheck from a scratch directory (chapter 7's demo writes HTML files) but took the build directory as a relative path, so every memcheck run failed to find its binary. | `realpath` on the build directory. |
| 4 | Unit tests | Several first-draft tolerances were tighter than the methods' truncation error (finite-difference vega with h = 1e-3, midpoint quadrature with the payoff kink inside the interval, the 100,000-period compounding gap). | Tolerances derived from the leading error term and commented; the compounding test now checks the gap against its asymptotic formula P e^{rT} r^2 T / (2m). |

Implied volatility (chapter 19) was additionally fuzzed over 191,436
random (strike, maturity, vol, rate) cases with time value above 1e-6:
no exceptions, every vol recovered to 1e-6. No change was needed.

## Performance: what the profiles found, and the fixes

| # | Benchmark / workload | Finding | Fix | Before | After |
| --- | --- | --- | --- | --- | --- |
| P1 | `BM_Ch09CallMc`, `BM_Ch10CallMc` (ch 9-10) | callgrind: `malloc`/`free` were **28%** of the `mc09` workload. Each scenario called `GeneratePricePath`, which allocated the path and a vector of normals, and built a fresh `std::normal_distribution`, which throws away the second normal of every Marsaglia-polar pair. | `PathGenerator`: per-step drift and volatility computed once, one distribution kept across paths, caller-owned buffer. Used by the chapter 9, 10, 14 and 20 pricers. | 5.9 ms / 50k paths; 90.0M Ir | **2.2-2.4 ms (2.6x)**; 28.1M Ir (-69%) |
| P2 | `BM_AsianMc`, `BM_BarrierMc` (ch 12) | After P1, path-dependent pricing ran 25-50% *slower* despite 5% fewer instructions: the first version fused "draw a normal" and "accumulate and exponentiate" into one loop, putting the polar method's rejection branch on the log S dependency chain. Found by the after-benchmarks, confirmed with old and new binaries run alternately. | Two passes into the same buffer (normals first, then the cumulative sum and `exp`), with a comment saying why. | 12 / 52 / 252 steps: 5.4 / 19-22 / 91 ms | 4.7-4.9 / 20-21 / 97-104 ms: parity (12 steps ~10% faster, 252 within noise); 111M vs 115M Ir |
| P3 | `BM_MatrixMultiply` (ch 16) | callgrind: 88% of `matmul` in `operator*`; the i-j-k loop walks a row of the column-major left operand with a stride of `rows()` doubles. cachegrind: 9.4% D1 write-miss rate. | j-k-i order: the inner loop runs down a column of `a` and of `c`, contiguous and vectorised. (callgrind Ir rose 12% at `-O2` because `c` is now updated in memory, but D1 misses fell from 5.9% to 4.0% and the time fell 3x.) | 64: 219 us; 256: 19.1 ms | **64: 76 us; 256: 5.6-6.2 ms (3.1x)** |
| P4 | `BM_Solve`, `BM_Cholesky` (ch 16) | Same stride problem: the LU update `lu(i, j) -= f * lu(k, j)` looped over j along a row, and Cholesky's inner products ran along rows of L. | Column-oriented (right-looking) LU elimination and back substitution; left-looking column Cholesky. | Solve 256: 16.8 ms; Cholesky 256: 3.8 ms | **Solve 2.7 ms (6.1x); Cholesky 1.2-1.5 ms (2.6-3.1x)** |
| P5 | `BM_PdeAmerican` (ch 21) | callgrind: 73% of `pde_am` in the projected SOR sweeps, which took tens of iterations per time step. | Brennan-Schwartz: the early-exercise floor is applied inside one tridiagonal sweep, in the direction that starts inside the exercise region (upward for puts, downward for calls). Exact for a single exercise boundary; a test shows it agrees with SOR at tolerance 1e-13 to 1e-9. SOR stays available as `AmericanSolver::kProjectedSor`. | 200: 6.0 ms; 800: 133 ms; 14.0M Ir | **200: 0.35 ms (17x); 800: 5.3 ms (25x)**; 3.4M Ir |
| P6 | `BM_PdeEuropean` (ch 21) | The tridiagonal coefficients were rebuilt and refactorised on every time step, and `SolveTridiagonal` allocated its scratch vector each call (11% of `pde_eu` in `memset`). | The two implicit matrices (Rannacher theta = 1, Crank-Nicolson theta = 1/2) are factorised once; each step is a forward and a backward sweep with multiplications by stored reciprocals. | 800: 7.5 ms; 11.6M Ir | **4.8 ms (1.55x)**; 7.5M Ir |
| P7 | `BM_CrrAmerican` (appendix A) | callgrind: `pow()` was **70%** of the `crr` workload, called at every exercisable node to rebuild S u^(2j-n). | A spot array: stepping back one level multiplies each node by u. The discount is folded into the two branch probabilities. | 2000 steps: 37.2 ms; 87.8M Ir | **1.6-1.75 ms (22x)**; 14.3M Ir |
| P8 | `BM_Prctile` (ch 5) | `Prctile` sorted a full copy to read two order statistics. | `nth_element` for the lower one and `min_element` over the upper partition for the next. | 262,144 values: 19.3 ms | **0.68 ms (28x)** |
| P9 | `BM_BasketMc` (ch 16) | callgrind: 23% of `basket` in the general matrix-vector product: the full n x n multiply by the lower-triangular Cholesky factor, plus two allocations per scenario. | A scenario-loop `SimulateTerminal` overload: only k <= i, persistent distribution, caller-owned scratch. | 2 stocks: 1.41 ms; 71.7M Ir | **1.0-1.1 ms (1.3-1.4x)**; 56.8M Ir (-21%) |

### Not changed

* `BM_DeltaHedging`: callgrind puts `erfc`, `log` and `exp` (one Black-Scholes
  delta per rebalancing date) at ~40% of the workload. They are the cost of
  the model; P1 removed the allocations (45.6M to 45.2M Ir), and the
  wall-clock change is within noise.
* `BM_IntegrateStdFunction` vs template vs virtual vs function pointer (the
  chapter 19 lesson): std::function costs ~15-25% over the others here; the
  integrand's `exp` dominates, so the template's inlining gains little. Left
  as it is, since the comparison is the point.
* `BM_DynamicMatrix4x4` (70 ns) vs `BM_FixedMatrix4x4` (19-23 ns): the
  difference is the heap allocation per product, which is what chapter 17's
  fixed-size class template is for.
* massif peaks: VaR 3.3 MB (the 200,000-loss vector and its copy for
  `nth_element`), every other workload below 100 KB.
* Parallel scaling (chapter 20): 1 / 2 / 4 threads at 47 / 24 / 12.7 ms,
  3.7x on 4 vCPUs; blocks are claimed with one relaxed `fetch_add` and each
  block's statistics are written once, so there is no false sharing to fix.
