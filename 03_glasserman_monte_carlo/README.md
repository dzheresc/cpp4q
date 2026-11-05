# Book 3: Monte Carlo Methods in Financial Engineering (Paul Glasserman, Springer 2003)

This is original C++20 code for each chapter, in Google C++ style, built with
CMake. It does not reproduce the book's text or algorithms listing by listing;
it implements the methods each chapter describes and checks every estimator
against a closed form (Black-Scholes, geometric Asian, Vasicek and CIR bonds,
Merton's series, binomial trees) or a much more accurate reference. Each
chapter is a small library, a `main.cc` demo and a GoogleTest suite.
Benchmarks, valgrind and the sanitizers cover the whole book.

| Chapter | Folder | What it shows |
| --- | --- | --- |
| 1. Foundations | `ch01_foundations/` | Black-Scholes, digital and discrete geometric-Asian closed forms with Greeks; `RunningStats` (Welford, mergeable) and `Estimate` with CLT intervals; plain MC for any payoff; the n^(-1/2) rate measured; work-normalised variance |
| 2. Random numbers and variables | `ch02_random_numbers/` | L'Ecuyer's MRG32k3a with O(log n) skip-ahead and 2^76-spaced substreams; MINSTD and RANDU (its 15 planes, tested); Beasley-Springer-Moro inverse normal (+ Halley refinement), Box-Muller, Marsaglia polar; exponential, Poisson, Marsaglia-Tsang gamma, chi-square and noncentral chi-square; Cholesky multivariate normal; `ParallelSimulate`, bit-identical for any thread count |
| 3. Sample paths | `ch03_sample_paths/` | random-walk and Brownian-bridge construction on any grid; exact GBM, correlated multi-asset GBM; Vasicek with the exact joint law of (r, integral of r); CIR by exact noncentral chi-square steps; Merton jump-diffusion paths and its series price |
| 4. Variance reduction | `ch04_variance_reduction/` | antithetics, control variates with the sample-optimal beta (underlying for calls, geometric Asian for arithmetic Asians: 1000x), stratified sampling (European, and terminal stratification of Asians through the bridge), importance sampling by mean shift for out-of-the-money calls |
| 5. Quasi-Monte Carlo | `ch05_quasi_monte_carlo/` | radical inverse, Halton, Sobol (Gray code, Joe-Kuo direction numbers, 16 dims, primitivity tested), random digital shift, Warnock's L2-star discrepancy; randomized QMC for an Asian with random-walk vs bridge paths |
| 6. Discretization | `ch06_discretization/` | Euler and Milstein steps; strong order 1/2 vs 1 measured on GBM; weak order 1 from exact Euler moments; Richardson extrapolation; two-point increments; Euler fixes for CIR (absorb, reflect, full truncation) |
| 7. Sensitivities | `ch07_sensitivities/` | finite differences with and without common random numbers (forward bias shown); pathwise delta and vega; likelihood-ratio delta, vega and gamma; why pathwise fails for digitals; Asian delta both ways |
| 8. American options | `ch08_american_options/` | CRR binomial (European, Bermudan, American) as the reference; Longstaff-Schwartz regression with in-sample (high) and out-of-sample (low) estimates; the Broadie-Glasserman random tree with its high and low estimators |
| 9. Risk management | `ch09_risk_management/` | option-book VaR and expected shortfall by full revaluation and by delta-gamma-theta; Jacobi diagonalisation of the quadratic; tail probabilities by mean-shift and by delta-gamma exponential twisting; Gaussian-copula credit losses with Glasserman-Li two-step importance sampling |

## Build

From the repository root:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
build/03_glasserman_monte_carlo/glasserman_ch04_variance_reduction_demo
build/03_glasserman_monte_carlo/bench/glasserman_bench    # Google Benchmark
```

Every random experiment takes a seed, so tests and demos are deterministic.
Statistical tests compare against the exact answer within 4 standard errors.

## Profiling

`profiling/run_valgrind.sh` runs memcheck over every test and demo, helgrind
and drd over the parallel workload, and callgrind, cachegrind and massif over
the fixed workloads in `profiling/profile_driver.cc`.
`profiling/run_sanitizers.sh` builds and runs everything under
AddressSanitizer + UndefinedBehaviorSanitizer and under ThreadSanitizer.
Findings, fixes and before/after timings are in
[`profiling/REPORT.md`](profiling/REPORT.md).
