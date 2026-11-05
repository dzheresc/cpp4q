# Book 4: C++ for Financial Mathematics (John Armstrong, CRC Press 2017)

This is original C++20 code for each chapter, in Google C++ style, built with
CMake. It does not reproduce the book's text or listings; it follows the
book's arc, from a first program to a threaded Monte Carlo engine, and
writes each chapter's ideas as a small library, a `main.cc` demo and a
GoogleTest suite. Every pricer is checked against a closed form
(Black-Scholes, digital, geometric Asian, continuous barrier with the
Broadie-Glasserman-Kou correction, Margrabe) or against an independent
method (quadrature, PDE vs binomial tree). Benchmarks, valgrind and the
sanitizers cover the whole book.

| Chapter | Folder | What it shows |
| --- | --- | --- |
| 1. Getting Started | `ch01_getting_started/` | a first program: simple, periodic and continuous compounding (`log1p`/`exp` form), equivalent rates, doubling time |
| 2. Basic Data Types and Operators | `ch02_data_types/` | checked int64 add/multiply, floor division and modulus, machine epsilon by halving (for float, double, long double), Neumaier compensated summation, `expm1` cancellation, `ApproxEqual` |
| 3. Functions | `ch03_functions/` | the "matlib" library: normal pdf/cdf, the Abramowitz-Stegun polynomial cdf (error < 7.5e-8, tested), Acklam inverse cdf with a Halley step (~1e-15), Black-Scholes prices and Greeks, a cancellation-free quadratic solver, overloading |
| 4. Flow of Control | `ch04_flow_of_control/` | factorials, primality and a sieve, `exp` by series with a do-while stopping rule, annuities by `switch`, bisection, midpoint and trapezium rules, a call priced by integrating against the lognormal density |
| 5. Working with Multiple Files | `ch05_multiple_files/` | matlib's vector half: linspace, mean, Welford std, min/max, MATLAB-compatible `Prctile`, `RandUniform`/`RandN` with an explicit generator |
| 6. Unit Testing | `ch06_unit_testing/` | a home-made test framework in C++20: assertion functions that capture the caller with `std::source_location`, self-registering tests, a runner that counts failures and errors |
| 7. Using C++ Classes | `ch07_using_classes/` | `std::string`, `std::vector`, streams: line and pie charts written as self-contained HTML/SVG, RFC 4180 CSV, HTML escaping |
| 8. User-Defined Types | `ch08_user_defined_types/` | `BlackScholesModel` (aggregate) and `CallOption`/`PutOption` (classes with invariants and const members) |
| 9. Monte Carlo Pricing | `ch09_monte_carlo/` | exact lognormal paths in the real-world and risk-neutral measures, `PathGenerator` for scenario loops, a pricer reporting standard errors |
| 10. Interfaces | `ch10_interfaces/` | `ContinuousTimeOption`, `PathIndependentOption`, `Priceable`, `RealFunction`; calls, puts and digitals; one Monte Carlo pricer for all; pricing by quadrature through the interface |
| 11. Arrays, Strings, and Pointers | `ch11_arrays_strings_pointers/` | pointer arithmetic, C strings (`strlcpy` semantics), `unique_ptr<double[]>`, `span`/`string_view`; a price CSV reader to log returns and historical volatility |
| 12. More Sophisticated Classes | `ch12_sophisticated_classes/` | abstract bases with data, the template-method pattern, `final`; up/down knock-in/out barriers and arithmetic/geometric Asians; closed forms (geometric Asian, Reiner-Rubinstein up-and-in/out, BGK discrete-monitoring shift); `operator<<` |
| 13. The Portfolio Class | `ch13_portfolio/` | positions sharing securities through `shared_ptr<const Priceable>`, stock and zero-coupon bond, bump-and-reprice delta/gamma/vega, the put-call parity portfolio (worth exactly zero) |
| 14. Delta Hedging | `ch14_delta_hedging/` | discrete delta hedging of a short call: mean P&L ~ 0, std falling like n^(-1/2) (tested at 16x rebalancing = 4x smaller), and the gamma P&L of hedging at the wrong vol |
| 15. Debugging and Development Tools | `ch15_debugging_tools/` | `Require` with source locations, `RequireFinite` for NaN hunting, a levelled `Logger` that skips formatting below threshold, `ScopedTimer`; the tools themselves are applied in `profiling/` |
| 16. A Matrix Class | `ch16_matrix/` | a column-major matrix owning a raw buffer (rule of five, copy-and-swap, noexcept moves), operators, transpose, Cholesky, LU with partial pivoting; a correlated multi-stock model pricing basket and exchange options (Margrabe) |
| 17. An Overview of Templates | `ch17_templates/` | concept-constrained Simpson integration for any floating type, `SumOf` any range, a constexpr `FixedMatrix<T, R, C>` with compile-time shape checks, a `LinearInterpolator` zero curve with a deduction guide |
| 18. The Standard Template Library | `ch18_stl/` | historical VaR and expected shortfall by `nth_element`, histograms in `std::map`, trade aggregation and `erase_if`, `partial_sort` for the largest exposures, a deque rolling mean, `std::set` of dates |
| 19. Function Objects and Lambdas | `ch19_function_objects/` | functors, capturing lambdas, the same integral through a function pointer, a virtual interface, `std::function` and a template; bisection and Newton; implied volatility (Newton from the Manaster-Koehler start, bisection fallback) |
| 20. Threads | `ch20_threads/` | parallel Monte Carlo with per-block seeded generators, an atomic block counter and `std::jthread`; results bit-identical for any thread count (tested); `std::async` and mutex variants |
| 21. Next Steps | `ch21_next_steps/` | the Black-Scholes PDE: Crank-Nicolson with Rannacher start-up, cell-averaged payoff, spot-aligned grid, prefactored tridiagonal solves, Brennan-Schwartz (and projected SOR) for American exercise, cross-checked against the binomial tree |
| A. Risk-Neutral Pricing | `appendix_a_risk_neutral/` | one-step replication equals the risk-neutral price, the martingale property on the tree, CRR European/American prices converging to Black-Scholes |

## Build

From the repository root:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
build/04_armstrong_cpp_financial_math/armstrong_ch12_sophisticated_classes_demo
build/04_armstrong_cpp_financial_math/bench/armstrong_bench    # Google Benchmark
```

Every random experiment takes a seed, so tests and demos are deterministic.
Statistical tests compare against the exact answer within 4 standard errors;
deterministic numerical methods are tested against their known order of
convergence.

## Profiling

`profiling/run_valgrind.sh` runs memcheck over every test and demo, helgrind
and drd over the threaded workload, and callgrind, cachegrind and massif over
the fixed workloads in `profiling/profile_driver.cc`.
`profiling/run_sanitizers.sh` builds and runs everything under
AddressSanitizer + UndefinedBehaviorSanitizer and under ThreadSanitizer.
Findings, fixes and before/after measurements are in
[`profiling/REPORT.md`](profiling/REPORT.md).
