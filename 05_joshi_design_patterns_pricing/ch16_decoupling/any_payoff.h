// Chapter 16: decoupling the interface from inheritance (type erasure).
//
// Chapters 3-4 made every pay-off derive from PayOff and implement Clone,
// coupling every product to that base class. AnyPayOff inverts it: any
// copyable type with `double operator()(double) const` is a pay-off, with
// no base class and no Clone; the inheritance (and the cloning) live inside
// AnyPayOff, where clients cannot see them. Copies are deep, so AnyPayOff
// is a plain value, like std::function but with the exact pay-off
// signature, copy-on-copy semantics and no allocation for an empty state.

#ifndef CPP4Q_JOSHI_CH16_DECOUPLING_ANY_PAYOFF_H_
#define CPP4Q_JOSHI_CH16_DECOUPLING_ANY_PAYOFF_H_

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

namespace cpp4q::joshi::ch16 {

template <class T>
concept PayOffValue =
    std::copy_constructible<T> && requires(const T& t, double s) {
      { t(s) } -> std::convertible_to<double>;
    };

class AnyPayOff {
 public:
  template <class T>
    requires(!std::same_as<std::decay_t<T>, AnyPayOff>) &&
            PayOffValue<std::decay_t<T>>
  AnyPayOff(T&& payoff)  // NOLINT: implicit, like std::function
      : self_(std::make_unique<Model<std::decay_t<T>>>(
            std::forward<T>(payoff))) {}

  AnyPayOff(const AnyPayOff& other) : self_(other.self_->Copy()) {}
  AnyPayOff& operator=(const AnyPayOff& other) {
    AnyPayOff tmp(other);
    self_.swap(tmp.self_);
    return *this;
  }
  AnyPayOff(AnyPayOff&&) noexcept = default;
  AnyPayOff& operator=(AnyPayOff&&) noexcept = default;
  ~AnyPayOff() = default;

  // Must not be called on a moved-from AnyPayOff.
  double operator()(double spot) const { return self_->Call(spot); }

 private:
  struct Concept {
    virtual ~Concept() = default;
    virtual double Call(double spot) const = 0;
    virtual std::unique_ptr<Concept> Copy() const = 0;
  };
  template <class T>
  struct Model final : Concept {
    explicit Model(T v) : value(std::move(v)) {}
    double Call(double spot) const override { return value(spot); }
    std::unique_ptr<Concept> Copy() const override {
      return std::make_unique<Model>(value);
    }
    T value;
  };

  std::unique_ptr<Concept> self_;
};

}  // namespace cpp4q::joshi::ch16

#endif  // CPP4Q_JOSHI_CH16_DECOUPLING_ANY_PAYOFF_H_
