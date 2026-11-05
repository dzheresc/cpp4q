# Book 1: Learning Modern C++ for Finance (Daniel Hanson, O'Reilly 2024)

This is original C++20 code for each chapter, in Google C++ style, built with
CMake. It does not reproduce the book's listings. Each chapter is a small
library, a `main.cc` demo and a GoogleTest suite. Benchmarks and valgrind
profiling cover the whole book.

| Chapter | Folder | What it shows |
| --- | --- | --- |
| 1. An Overview of C++ | `ch01_overview/` | free functions, `enum class`, `constexpr`, `std::numbers`, designated initialisers: Black-Scholes-Merton price and Greeks |
| 2. User-Defined Classes | `ch02_classes/` | invariants, `[[nodiscard]]`, defaulted `operator<=>`, hidden friends; `PriceBuffer` spells out all five special members (Appendix C) |
| 3. Inheritance, Polymorphism, Smart Pointers | `ch03_polymorphism/` | abstract `Payoff` with `Clone()` and a protected copy to stop slicing; `unique_ptr` ownership; `shared_ptr<const YieldCurve>`; a `std::variant` alternative |
| 4. STL Containers and Iterators | `ch04_stl_containers/` | `std::map` order book (bids via `std::greater<>`), heterogeneous lookup, a ring-buffer `RollingWindow` with its own forward iterator |
| 5. STL Algorithms and Ranges | `ch05_algorithms_ranges/` | `transform`, `transform_reduce`, `inclusive_scan`, `nth_element`, `partial_sort_copy` with projections, views pipelines |
| 6. Random Numbers and Concurrency | `ch06_random_concurrency/` | `<random>` with `seed_seq` streams, antithetic GBM Monte Carlo, `std::async` and `std::jthread` parallel pricers |
| 7. Dates and Fixed Income | `ch07_dates_fixed_income/` | `std::chrono::year_month_day`, business-day conventions, day counts (ACT/360, ACT/365F, 30/360, 30E/360, ACT/ACT ISDA), schedules, bond price, yield, duration and convexity |
| 8. Linear Algebra | `ch08_linear_algebra/` | dense `Matrix`, naive and cache-tiled products, Cholesky, LU with pivoting, correlated normals; `std::valarray` slices (Appendix E) |
| 9. The Boost Libraries | `ch09_boost/` | Boost.Math `normal`, TOMS 748 and Newton implied vol, Gauss-Kronrod pricing by quadrature |
| 10. Modules and Concepts | `ch10_modules_concepts/` | `PayoffFunction` / `DiscountCurve` / `CashFlowRange` concepts, a generic CRR binomial tree (European/American, any `floating_point`), a named module `cpp4q.hanson.pricing` |
| Appendices A-D | `appendices/` | virtual destructor, object slicing, vector brace initialisation |

## Build

From the repository root:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build                                  # 67 tests
build/01_hanson_modern_cpp_finance/hanson_ch06_random_concurrency_demo
build/01_hanson_modern_cpp_finance/bench/hanson_bench   # Google Benchmark
```

GoogleTest and Google Benchmark come from the system if installed and are
fetched otherwise. Boost (headers only) is optional; without it, chapter 9
is skipped.

The named-module example needs Ninja and clang 17 or newer (or gcc 14 or newer):

```sh
CXX=clang++ cmake -S . -B build-clang -G Ninja -DCPP4Q_ENABLE_MODULES=ON
cmake --build build-clang && build-clang/01_hanson_modern_cpp_finance/hanson_ch10_module_demo
```

## Profiling

`profiling/run_valgrind.sh` runs memcheck, helgrind, drd, callgrind,
cachegrind and massif. [`profiling/REPORT.md`](profiling/REPORT.md) lists
what they found and the fixes, with before and after numbers. For example,
the American binomial tree is 19x faster, matmul is 5.8x and the moving
average is 158x.
