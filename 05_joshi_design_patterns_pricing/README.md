# Book 5: C++ Design Patterns and Derivatives Pricing (Mark S. Joshi, 2nd ed., Cambridge 2008)

This is original C++20 code for each chapter, in Google C++ style, built with
CMake. It does not reproduce the book's text or listings; it follows the
book's arc, from a twenty-line Monte Carlo function to a pricing library
whose products, models, random number generators and statistics are all
interchangeable, and writes each chapter's idea as a small library, a
`main.cc` demo and a GoogleTest suite.

The book was written for C++98, so its idioms are modernised where the
language now does the job: `Clone()` returns `std::unique_ptr`, the bridge
and `Wrapper<T>` hold a `unique_ptr` (copy by cloning, `noexcept` moves),
`MJArray` arguments become `std::span`, solvers are constrained by
concepts, the factory holds `std::function` creators behind a
`std::shared_mutex`, and the hand-rolled guards of the exceptions chapter
are replaced by RAII. The patterns themselves are kept, because they are
the point of the book. Every pricer is tested against a closed form
(Black-Scholes, digital, double digital, discrete geometric Asian,
continuous down-and-out with the Broadie-Glasserman-Kou shift) or against
an exact property (put-call parity, zero-vol forwards, discounting of
time-dependent rates).

| Chapter | Folder | What it shows |
| --- | --- | --- |
| 1. A simple Monte Carlo model | `ch01_simple_monte_carlo/` | a call priced by sampling S(T) directly; Gaussians by summing 12 uniforms (kurtosis 2.9, tested), Box-Muller and the polar method; an explicit `std::mt19937_64` instead of `rand()` |
| 2. Encapsulation | `ch02_encapsulation/` | a `PayOff` class with an enum and a switch: encapsulated, but not closed for modification |
| 3. Inheritance and virtual functions | `ch03_inheritance/` | an abstract `PayOff` (virtual destructor, protected copy so it cannot slice); call, put and a double digital added without touching the pricer |
| 4. Bridging with a virtual constructor | `ch04_virtual_constructor/` | `Clone()` returning `unique_ptr`, a CRTP `Cloneable` helper, `PayOffBridge` (value semantics, strong-guarantee assignment), `VanillaOption`, constant and piecewise-constant `Parameters` behind the same bridge |
| 5. Strategies, decoration, and statistics | `ch05_strategies_statistics/` | the generic `Wrapper<T>`; `StatisticsMc` strategies (mean; one-pass mean, variance, skew and kurtosis; quantiles) and the `ConvergenceTable` decorator |
| 6. A random numbers class | `ch06_random_numbers/` | `RandomBase` with skip/reseed/reset, Park-Miller (checked against the published 10,000th value, with O(log n) jump-ahead), the `AntiThetic` decorator, and a Halton sequence behind the same interface |
| 7. An exotics engine and the template pattern | `ch07_exotics_engine/` | path-dependent products (arithmetic and geometric Asians, a discrete knock-out with rebate paid at the knock date) and `ExoticEngine::DoSimulation` as a template method; exact log-normal steps with time-dependent r, d, vol |
| 8. Trees | `ch08_trees/` | `TreeProduct` (European, American) and the book's equal-probability log-space binomial tree; one tree prices any product; O(steps) storage and one virtual call per layer rather than per node |
| 9. Solvers, templates, and implied volatilities | `ch09_solvers_implied_vol/` | bisection on any callable, Newton-Raphson with pointer-to-member template parameters (the book's form) and with callables; implied vol from the Manaster-Koehler start, randomly tested over 2,000 cases |
| 10. The factory | `ch10_factory/` | a singleton `PayOffFactory` and self-registering `PayOffHelper<T>`; a new pay-off added in one file; why static libraries drop registrars and the `WHOLE_ARCHIVE` link that keeps them |
| 11. Design patterns revisited | `ch11_patterns_revisited/` | adapter (any standard engine as a `RandomBase`), composite (`PayOffPortfolio`: a butterfly priced in one simulation), decorator (`PayOffCollar`) |
| 12. The situation in 2007 | `ch12_situation_2007/` | the ways to hand a pay-off to a pricer today (virtual per spot or per batch, `std::function`, concept-constrained template, `std::variant`) giving identical numbers, and what each costs; `shared_ptr<const PayOff>` sharing instead of cloning |
| 13. Exceptions | `ch13_exceptions/` | basic vs strong guarantee demonstrated with a pay-off whose `Clone` fails at every possible point; copy-and-swap; `noexcept` moves; `ScopeExit`; a boundary mapping exceptions to status codes |
| 14. Templatizing the factory | `ch14_template_factory/` | `Factory<Base, Args...>` and `Registrar<Base, Derived, Args...>`; pay-offs from a strike, from two barriers, and generators from (dimension, seed); concurrent lookups tested with threads |
| 15. Interfacing with EXCEL | `ch15_excel_interface/` | the xlw architecture without Excel: cell and range types, argument conversion with named errors, a function registry with help text, errors returned as cells, and a C ABI (`extern "C"`, status codes, thread-local last error) |
| 16. Decoupling | `ch16_decoupling/` | a pimpl compilation firewall (`McPricer`: its header includes only standard headers; its test includes nothing else) and type erasure (`AnyPayOff`: any callable is a pay-off, no base class) |
| A. Black-Scholes formulas | `appendix_a_black_scholes/` | call, put, digitals, delta, gamma, vega, with zero-vol and zero-time limits |
| B. Distribution functions | `appendix_b_distributions/` | normal density and cdf (erfc), the Beasley-Springer-Moro inverse (< 3e-9, tested) and a Halley-refined inverse |
| C. A simple array class | `appendix_c_array/` | `MjArray`: size vs capacity, element-wise and scalar arithmetic, `std::span` views |

## Build

From the repository root:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
build/05_joshi_design_patterns_pricing/joshi_ch07_exotics_engine_demo
build/05_joshi_design_patterns_pricing/bench/joshi_bench    # Google Benchmark
```

Every random experiment takes a seed, so tests and demos are deterministic.
Statistical tests compare against the exact answer within 4 standard errors.

## Profiling

`profiling/run_valgrind.sh` runs memcheck over every test and demo, helgrind
and drd over the threaded factory workload, and callgrind, cachegrind and
massif over the fixed workloads in `profiling/profile_driver.cc`.
`profiling/run_sanitizers.sh` builds and runs everything under
AddressSanitizer + UndefinedBehaviorSanitizer and under ThreadSanitizer.
Findings, fixes and before/after measurements are in
[`profiling/REPORT.md`](profiling/REPORT.md).
