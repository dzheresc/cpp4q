# cpp4q

C++ for quantitative finance: a reading list with chapter-level tables of contents, and C++20 code that works through each book in order.

## Code

Each book has its own top-level folder, and they all build from the root `CMakeLists.txt`:

| Folder | Book | Status |
| --- | --- | --- |
| [`01_hanson_modern_cpp_finance/`](01_hanson_modern_cpp_finance/) | Hanson, *Learning Modern C++ for Finance* | all 10 chapters + appendices, tests, benchmarks, [profiling report](01_hanson_modern_cpp_finance/profiling/REPORT.md) |
| [`02_williams_cpp_concurrency/`](02_williams_cpp_concurrency/) | Williams, *C++ Concurrency in Action* | all 11 chapters + appendix C, tests, benchmarks, ThreadSanitizer, [profiling report](02_williams_cpp_concurrency/profiling/REPORT.md) |

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build
```

Requirements: a C++20 compiler (GCC 13+ or Clang 17+), CMake 3.25+, optionally Boost headers, oneTBB (parallel STL backend) and valgrind. `-DCPP4Q_SANITIZE=thread` (or `address,undefined`) builds everything with a sanitizer. GoogleTest and Google Benchmark are fetched automatically when they are not installed.

Books are the five recommended in [Best C++ Quantitative Finance Books](https://cppforquants.com/best-c-quantitative-finance-books/) (cppforquants.com).

| # | Title | Author(s) | Edition |
| --- | --- | --- | --- |
| 1 | [Learning Modern C++ for Finance](#1-learning-modern-c-for-finance) | Daniel Hanson | 1st (O'Reilly, 2024) |
| 2 | [C++ Concurrency in Action](#2-c-concurrency-in-action-2nd-ed) | Anthony Williams | 2nd (Manning, 2019) |
| 3 | [Monte Carlo Methods in Financial Engineering](#3-monte-carlo-methods-in-financial-engineering) | Paul Glasserman | 1st (Springer, 2003) |
| 4 | [C++ for Financial Mathematics](#4-c-for-financial-mathematics) | John Armstrong | 1st (CRC Press, 2017) |
| 5 | [C++ Design Patterns and Derivatives Pricing](#5-c-design-patterns-and-derivatives-pricing-2nd-ed) | Mark S. Joshi | 2nd (Cambridge, 2008) |

## 1. Learning Modern C++ for Finance

*Daniel Hanson* · Source: [O'Reilly](https://www.oreilly.com/library/view/learning-modern-c/9781098100797/)

1. An Overview of C++
2. Writing User-Defined Classes with Modern C++ Features
3. Inheritance, Polymorphism, and Smart Pointers
4. The Standard Template Library Part I: Containers and Iterators
5. The Standard Template Library Part II: Algorithms and Ranges
6. Random Number Generation and Concurrency
7. Dates and Fixed Income Securities
8. Linear Algebra
9. The Boost Libraries
10. Modules and Concepts

Appendices: A. Virtual Default Destructor · B. Object Slicing · C. Implementation of Move Special Member Functions · D. Resolving Conflicts in the Initialization of a vector · E. valarray and Matrix Operations

## 2. C++ Concurrency in Action, 2nd ed.

*Anthony Williams* · Source: [O'Reilly / Manning](https://oreilly.com/library/view/c-concurrency-in/9781617294693/kindle_split_003.html)

1. Hello, world of concurrency in C++!
2. Managing threads
3. Sharing data between threads
4. Synchronizing concurrent operations
5. The C++ memory model and operations on atomic types
6. Designing lock-based concurrent data structures
7. Designing lock-free concurrent data structures
8. Designing concurrent code
9. Advanced thread management
10. Parallel algorithms
11. Testing and debugging multithreaded applications

Appendices: A. Brief reference for some C++11 language features · B. Brief comparison of concurrency libraries · C. A message-passing framework and complete ATM example · D. C++ Thread Library reference

## 3. Monte Carlo Methods in Financial Engineering

*Paul Glasserman* · Source: [Eyrolles listing](https://www.eyrolles.com/Loisirs/Livre/monte-carlo-methods-in-financial-engineering-9780387004518) (Springer, Stochastic Modelling and Applied Probability vol. 53)

1. Foundations
2. Generating Random Numbers and Random Variables
3. Generating Sample Paths
4. Variance Reduction Techniques
5. Quasi-Monte Carlo
6. Discretization Methods
7. Estimating Sensitivities
8. Pricing American Options
9. Applications in Risk Management

Appendices follow; the source does not list their titles.

## 4. C++ for Financial Mathematics

*John Armstrong* · Source: [Perlego](https://perlego.com/book/1575609/c-for-financial-mathematics-pdf)

1. Getting Started
2. Basic Data Types and Operators
3. Functions
4. Flow of Control
5. Working with Multiple Files
6. Unit Testing
7. Using C++ Classes
8. User-Defined Types
9. Monte Carlo Pricing in C++
10. Interfaces
11. Arrays, Strings, and Pointers
12. More Sophisticated Classes
13. The Portfolio Class
14. Delta Hedging
15. Debugging and Development Tools
16. A Matrix Class
17. An Overview of Templates
18. The Standard Template Library
19. Function Objects and Lambda Functions
20. Threads
21. Next Steps

Appendix: A. Risk-Neutral Pricing

## 5. C++ Design Patterns and Derivatives Pricing, 2nd ed.

*Mark S. Joshi* · Source: [Cambridge University Press front matter (PDF)](https://assets.cambridge.org/97805217/21622/frontmatter/9780521721622_frontmatter.pdf)

1. A simple Monte Carlo model
2. Encapsulation
3. Inheritance and virtual functions
4. Bridging with a virtual constructor
5. Strategies, decoration, and statistics
6. A random numbers class
7. An exotics engine and the template pattern
8. Trees
9. Solvers, templates, and implied volatilities
10. The factory
11. Design patterns revisited
12. The situation in 2007
13. Exceptions
14. Templatizing the factory
15. Interfacing with EXCEL
16. Decoupling

Appendices: A. Black–Scholes formulas · B. Distribution functions · C. A simple array class · D. The code · E. Glossary
