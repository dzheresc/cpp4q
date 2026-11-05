# Book 5 profiling report

Environment: 4-vCPU shared cloud container, GCC 13.3 (and Clang 18 for a
warning-clean cross-check), glibc 2.39, valgrind 3.22. Benchmarks are `-O3`
(Release, median of 3 runs before and 5 after); valgrind runs on `-O2 -g`
(RelWithDebInfo); the sanitizers run on RelWithDebInfo builds with
`-DCPP4Q_SANITIZE=...`.

Wall-clock timings on this machine are noisy. Benchmarks whose code did not
change at all moved by 15-25% between runs (`BM_SimpleMonteCarlo5` 6.3 ->
7.6 ms, `BM_Gaussian<BoxMuller>` 43 -> 53 ns, `BM_MjArrayAxpy/64` 100 ->
191 ns), and three back-to-back runs of the Monte Carlo benchmarks spread
by as much. So only changes well beyond that are claimed below, and each is
backed by callgrind instruction counts (Ir), which are deterministic.

Reproduce:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
build/05_joshi_design_patterns_pricing/bench/joshi_bench --benchmark_repetitions=5
cmake -S . -B build-prof -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCPP4Q_BUILD_BENCHMARKS=OFF && cmake --build build-prof
05_joshi_design_patterns_pricing/profiling/run_valgrind.sh build-prof
05_joshi_design_patterns_pricing/profiling/run_sanitizers.sh
```

`profile_driver.cc` holds the 11 fixed workloads that valgrind runs.
Raw outputs: `results/bench_{before,after}.txt`,
`results/valgrind_summary_{before,after}.txt`,
`results/sanitizers_summary.txt`.

## Correctness tools: final state

| Tool | Target | Result |
| --- | --- | --- |
| memcheck (`--leak-check=full --track-origins=yes`) | 19 test binaries + 19 demos | 0 errors, 0 bytes lost, no suppressions (before and after the fixes) |
| helgrind | chapter 14: three threads creating generators through the `shared_mutex`-guarded factory while a fourth registers and unregisters | 0 errors |
| drd | same | 0 errors |
| AddressSanitizer + UndefinedBehaviorSanitizer | 19 test binaries + 19 demos | 0 reports, all pass |
| ThreadSanitizer | 19 test binaries + 19 demos (incl. the concurrent factory test) | 0 reports, all pass |

The pimpl/firewall design of chapter 16, the strong-guarantee code of
chapter 13 (a pay-off whose `Clone` throws `bad_alloc` at every possible
point) and the exception-to-error-cell boundary of chapter 15 all run under
memcheck and ASan, so their failure paths are checked for leaks too.

## Correctness issues found, and fixes

| # | Found by | Issue | Fix |
| --- | --- | --- | --- |
| 1 | Unit test (segfault) | The chapter 15 test helper returned a pointer into the `CellMatrix` returned by `Invoke`, a temporary destroyed at the end of the full expression: a segfault in the first run. The library was right, the test was not, but it is exactly the dangling-reference trap a C API boundary invites. | The helper returns the string by value, with a comment saying why. |
| 2 | Unit test | `AnyPayOff`'s converting constructor was constrained `PayOffValue<T> && !same_as<T, AnyPayOff>`. Checking `copy_constructible<AnyPayOff>` from inside AnyPayOff's own copy made constraint satisfaction recursive (GCC: "depends on itself"). | The `!same_as` test comes first, so the conjunction short-circuits before the recursive check. |
| 3 | Unit tests | First-draft tolerances tighter than the method: the binomial tree's error is first order but oscillates (steps x error ranges over -2..2 as the strike moves between nodes), and the finite-difference vega had an h^2/6 truncation of 3e-6. A Park-Miller skip test counted one vector too few; a Newton "zero derivative" test started at a point where f' was only approximately zero. | Bounds derived from the leading error term and commented (3/steps for the tree); the tests fixed. |
| 4 | Investigation | The chapter 7 demo's geometric Asian (antithetic Park-Miller, seed 7) sat 0.04 from the closed form, 1.8 naive standard errors. 20 seeds x 4 generator set-ups showed mean errors of -2.2, -0.5, -0.6 and +2.0 standard errors: no bias, no change needed. | None. |

## Performance: what the profiles found, and the fixes

| # | Benchmark / workload | Finding | Fix | Before | After |
| --- | --- | --- | --- | --- | --- |
| P1 | `BM_Tree*` (ch 8), `tree` | massif: **8.1 MB** peak heap for a 1000-step tree: the book's design keeps every node's (spot, value) pair in a `vector<vector<pair>>`. callgrind: `exp` was 35% of the workload, one per node. | The spot at (i, k) is `lowest_spot[i] * up^k`, with the powers precomputed (re-anchored by an exact `exp` every 64 steps so rounding cannot drift); one O(steps) value buffer updated in place. | European 2000 steps: 56.9 ms; American: 49.4 ms; 75.9M Ir; 8.1 MB | see P2 |
| P2 | `BM_TreeAmericanReprice` | After P1, two virtual calls per node (`PreFinalValue`, then the pay-off inside it) were what was left. | `TreeProduct` gained layer-level virtuals (`FinalPayOffs`, `PreFinalValues`) whose defaults loop over the per-node ones, so existing products still work (tested); `TreeAmerican` evaluates the exercise values of a whole layer with one batched pay-off call (P3) and a vectorised max. | reprice 2000: 13.0 ms | **2.7 ms (4.7x)**; with P1: European 2000 **1.5 ms (38x)**, American 2000 **2.8 ms (18x)**; 23.7M Ir (-69%); **123 KB** heap (-98%) |
| P3 | `BM_Dispatch*` (ch 12) | A virtual call per spot costs 5.5x a template or variant (509 vs 88-92 us for 65,536 spots): the call blocks inlining and vectorisation of a two-instruction pay-off. | `PayOff::Evaluate(span, span)`: still one virtual call, but per batch, and the CRTP `Cloneable` base implements it with a statically bound call to `Derived::operator()`, so every concrete pay-off gets the vectorised loop for free. `MeanPayOffVirtualBatched` shows it next to the others. | virtual: 459-509 us | **batched virtual: 107-113 us (4.3x)**, within 30% of the template |
| P4 | `BM_ConvergenceTable` (ch 5), `mc5`, `pimpl` | callgrind: `StatisticsMoments::DumpOneResult` was **58 Ir per result** (16% of `mc5`, 21% of `pimpl`): the Welford/Terriberry update of four central moments, with a division. | Power sums of (x - first result): four multiply-adds, no division; central moments reconstructed once at the end. Shifting keeps the large-mean case exact (tested with mean 1e9) and a test with a 50-sigma first result still matches a two-pass computation to 1e-7. | 10.7 ms; mc5 72.8M Ir | **6.4 ms (1.7x)**; 23 Ir per result; mc5 65.8M Ir, pimpl 27.6M -> 23.3M Ir |
| P5 | `BM_ParkMillerSkip` (ch 6) | `Skip(n)` drew and threw away n vectors (and allocated a scratch vector per call): O(n), 3 ms to skip 10,000 52-dimensional paths. | `ParkMiller::Discard`: multiply the state by a^n mod m by square-and-multiply, O(log n), exact (tested against drawing, and a full period returns to the start). | 61 us / 2969 us | **0.06 us / 0.09 us** |
| P6 | `asian`, `knockout`, `mc6` | callgrind: `RandomParkMiller::GetUniforms` was 21 Ir per uniform. Schrage's method (from the book) avoids 32-bit overflow at the price of a division. | In 64 bits the product a x < 2^46 fits, and m = 2^31 - 1 is a Mersenne prime, so the reduction is mask + shift + add. Same sequence bit for bit (the published 10,000th value test still passes). | 11.03M Ir per 520k uniforms | **7.89M Ir (-28%)**; asian 92.9M -> 89.8M, knockout 94.7M -> 91.2M Ir total; wall clock within noise |
| P7 | `BM_Mc6Halton` (ch 6) | Halton was 2.4x slower than Park-Miller: the radical inverse did a 64-bit division per digit and a floating multiply per digit. | Base 2 by bit reversal; other bases with 32-bit divisions into an integer numerator and one final floating division (also exactly rounded). Tested against the generic loop. | 13.1 ms | **3.35 ms (3.9x)** |
| P8 | `array`, `BM_MjArrayAxpy` (appendix C) | callgrind: 22% of `array` in `std::fill_n` inside `MjArray(size, value)`: the copy constructor delegated to it, zero-filling a buffer the copy then overwrote. And `y += a * x` makes a temporary per call (the book's operators). | Copy constructor writes the buffer once. `AddScaled(a, x)` added for the temporary-free form (the problem expression templates solve in general). | 4096: 7.9 us; 115.3M Ir | operators **4.7-5.9 us**, 89.0M Ir (-23%); `AddScaled` **1.5-2.4 us** |

### Not changed

* `InverseCumulativeNormal` (Moro) is now the top entry of every
  path-dependent profile at ~30%, followed by `exp` at ~30%. Both are the
  model: one Gaussian and one exponential per time step. A cheaper inverse
  (or Box-Muller) would change the numbers the book's antithetic and
  low-discrepancy generators produce, which is the point of using the
  inverse; the `exp` is needed because the products are written in spot,
  not log-spot.
* `BM_ImpliedVolNewton` (0.5-0.6 us) vs bisection (2.5-3.1 us): converges in
  3-5 iterations from the Manaster-Koehler start; the cost is `erfc`, `exp`
  and `log` inside the Black-Scholes formula.
* `BM_FactoryCreate` (54-58 ns): a shared lock, a `std::map` lookup by
  string, and the `make_unique` of the product; creators run outside the
  lock on purpose (so a creator may use the factory). Factories are called
  once per trade, not per path.
* `BM_BridgeCopy` (17-20 ns): one allocation per clone. That is the price of
  value semantics for polymorphic objects; chapter 12's
  `shared_ptr<const PayOff>` is the alternative when the pay-off is
  immutable.
* `std::function` dispatch (495-590 us) is as slow as the virtual call, and
  `AnyPayOff` (136-156 us) is faster than both because its model class is
  `final` and GCC devirtualises the call speculatively; neither was changed,
  since the comparison is the chapter's lesson.
* massif: every workload other than the tree peaks below 80 KB.
