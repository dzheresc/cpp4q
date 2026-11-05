# Book 3 profiling report

Environment: 4-vCPU shared cloud container, GCC 13.3, glibc 2.39,
valgrind 3.22. Benchmarks are `-O3` (Release, median of 3 runs before and
5 after); valgrind runs on `-O2 -g` (RelWithDebInfo); the sanitizers run on
RelWithDebInfo builds with `-DCPP4Q_SANITIZE=...`.

Wall-clock timings on this machine move by 10-20% between runs: benchmarks
whose code did not change (`BM_MertonCall`, `BM_AsianDelta`, `BM_SobolNext`)
came out 15-20% slower in the "after" run. Only changes well beyond that
are claimed below, and the callgrind instruction counts (deterministic)
back each one.

Reproduce:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
build/03_glasserman_monte_carlo/bench/glasserman_bench --benchmark_repetitions=5
cmake -S . -B build-prof -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCPP4Q_BUILD_BENCHMARKS=OFF && cmake --build build-prof
03_glasserman_monte_carlo/profiling/run_valgrind.sh build-prof
03_glasserman_monte_carlo/profiling/run_sanitizers.sh
```

`profile_driver.cc` holds the 15 fixed workloads that valgrind runs, at
least one per chapter. Raw outputs: `results/bench_{before,after}.txt`,
`results/valgrind_summary_{before,after}.txt`,
`results/sanitizers_summary.txt`.

## Correctness tools: final state

| Tool | Target | Result |
| --- | --- | --- |
| memcheck (`--leak-check=full --track-origins=yes`) | 9 test binaries + 9 demos | 0 errors, 0 bytes lost, no suppressions |
| helgrind | `ParallelSimulate`, 4 threads | 0 errors |
| drd | same | 0 errors |
| AddressSanitizer + UndefinedBehaviorSanitizer | 9 test binaries + 9 demos | 0 reports, all pass |
| ThreadSanitizer | 9 test binaries + 9 demos | 0 reports, all pass |

Book 2's lesson (spin loops that stall valgrind) did not apply: the only
concurrency here is `ParallelSimulate`, where workers claim blocks with one
`fetch_add` and publish through `jthread` join, which helgrind and drd both
model.

## Correctness issues found, and fixes

| # | Found by | Issue | Fix |
| --- | --- | --- | --- |
| 1 | Unit test | The Merton jump-diffusion series price was 2% too high: it weighted the conditional Black-Scholes prices by Poisson(lambda' T) **and** re-discounted from r_n to r. Either one alone is right; both double-count the compensator. The Monte Carlo price was the one that disagreed with it, at 4 standard errors. | Poisson(lambda' T) weights only; a test checks MC against the series at three strikes. |
| 2 | Unit test | The mean-shift importance sampler for portfolio VaR made the estimator **worse** than plain MC (standard error 2.3x larger) on a short-straddle book: the linearised loss has almost no delta, and the large losses lie in two opposite directions. | Added Glasserman-Heidelberger-Shahabuddin delta-gamma exponential twisting (Jacobi diagonalisation of the quadratic, twisted means and variances, theta from psi'(theta) = x). It is now the default; mean shift stays as the cautionary option, and a test shows twisting beating both. |
| 3 | Code review | `GaussianCopulaPortfolio::SolveTheta` returned theta = 2^20 when the loss threshold was at or above the total exposure, so the twisted default probabilities became `exp(2^20 c)` = inf and the estimate NaN. | Throws `std::invalid_argument`; tested. |
| 4 | Code review | `LsmPricer` left zero regression coefficients on dates with too few in-the-money paths, so a continuation value of 0 meant "always exercise" there, contradicting the stated "never exercise". | A per-date `fitted_` flag: no exercise where no regression was fitted, in both the in-sample and out-of-sample passes. |

## Performance: what the profiles found, and the fixes

| # | Benchmark / workload | Finding | Fix | Before | After |
| --- | --- | --- | --- | --- | --- |
| P1 | `BM_ParallelCall` (ch 2) | 4 threads ran only **1.4x** faster than 1. Each worker called `partial[b].Add()` once per path, and neighbouring `RunningStats` (24 bytes each) share cache lines across threads: false sharing on every path. | Accumulate in a stack-local `RunningStats` per block, store it once. Results stay bit-identical across thread counts (tested). | 1 / 4 threads: 109 / 78.6 ms | **97 / 25.2 ms (3.9x scaling)** |
| P2 | `BM_CrrAmerican` (ch 8) | callgrind: `pow()` was 9.4% of the whole `lsm` workload, all from the binomial tree, which recomputed `S0 u^(2j-n)` with `pow()` at every exercisable node. | Keep a spot array: stepping back one level multiplies node j by u. Also folds the discount into the two probabilities. | 4000 steps: 175 ms | **6.8 ms (26x)** |
| P3 | `BM_CreditTailIs` (ch 9) | callgrind: `SolveTheta` plus its `exp()` calls were ~80% of the credit workload. It bisected 100 times per path over 100 obligors. | Safeguarded Newton (psi'' is a by-product of psi'), converging in ~5 iterations. | 272 ms; 622M Ir | **87 ms; 187M Ir (3.3x)**; `FindFactorShift` 5.9 to 2.3 ms |
| P4 | `BM_SamplerNormal` (ch 2) | Every normal draw paid two out-of-line calls (`Sampler::Normal`, then `InverseNormalCdf`) plus a switch. | `InverseNormalCdf` and the inversion branch of `Normal()` are inline in the header; `FillNormal` hoists the method switch out of its loop. | 37.4 M normals/s | **56-61 M/s (1.5x)**; `BM_StrongError` Euler 1.5x |
| P5 | `BM_Mrg32k3aUniform` (ch 2) | Inlining `Mrg32k3a::NextUniform()` as well was measured **slower**: 97-100 M/s inlined vs 129-134 M/s out of line, in two separate 5-7-repetition runs (GCC 13 keeps the six state words in memory across the caller's loop). A floating-point MRG32k3a and an unsigned-modulo variant were also slower in a micro-benchmark (25 and 9 ns vs 7 ns). | Left out of line, with a comment saying why. | 132 M/s | 129-134 M/s (unchanged, by design) |
| P6 | `vr` workload (ch 4) | callgrind: `log()` was 20% of the variance-reduction workload; the geometric-average control took the log of each of 12 prices that had just been built as `exp(log path)`. | `AsianPath` builds the log path itself and sums it. | 235M Ir | **185M Ir (-21%)**; wall-clock change within noise |
| P7 | `BM_LsmTrain` / `BM_LsmPrice` (ch 8) | The path matrix was path-major, so the backward sweep read one date across all paths with a stride of `dates * 8` bytes; cachegrind reported a 1.3% D1 miss rate (valgrind's simulated LL is large enough to hide the rest). Out-of-sample pricing also stored the full matrix just to read it once. | Date-major matrix (each date's row streams); out-of-sample pricing simulates each path on the fly and stops at exercise, so it needs no matrix and draws no normals past the stopping date. | 50 dates: train 227 ms, price 94 ms; 685M Ir; D1 miss 1.3% | **train 170 ms, price 75 ms; 530M Ir; D1 miss 0.6%** |

### Not changed

* `SamplePoisson` with large means (4.3 ms per 4096 draws at mean 500)
  uses chunked sequential inversion. It is exact and only the jump-
  diffusion and CIR samplers use it, with means below ~20.
* `RandomTreePrice` allocates a b-element vector per internal node; massif
  peaks at 78 KB and callgrind shows `malloc` below 1%, so pre-allocating
  per-level scratch buys nothing measurable.
* massif peaks: LSM 8.2 MB (the 20,000 x 50 training matrix, which the
  regression needs), VaR 394 KB (the loss vector), QMC and tree 78 KB.

## Variance reduction: efficiency (variance x time)

Glasserman's comparison measure is variance per unit of work. From the
"after" benchmarks (200,000 payoff evaluations each, standard error
reported by each estimator):

| Contract | Estimator | Std error | Time | Efficiency gain vs plain |
| --- | --- | --- | --- | --- |
| ATM call | plain | 0.0329 | 10.5 ms | 1x |
| | antithetic | 0.0232 | 6.3 ms | 3.3x |
| | control variate (S(T)) | 0.0125 | 10.8 ms | 6.7x |
| | stratified (200 strata) | 0.00174 | 7.5 ms | ~500x |
| Asian, 12 fixings | plain | 0.0190 | 80 ms | 1x |
| | terminal stratification | 0.00967 | 89 ms | 3.5x |
| | geometric control variate | 0.00053 | 92 ms | ~1100x |

Randomized QMC (chapter 5 tests and demo) gives a 7.6x (random walk) to
18x (Brownian bridge) smaller standard error than MC for the 16-fixing
Asian at the same number of paths, at about the same cost per path.
