// Chapter 5: a generic deep-copying wrapper.
//
// Chapter 4 wrote one bridge per hierarchy. Wrapper<T> writes it once:
// it owns a T through std::unique_ptr and copies by calling T::Clone(), so
// any hierarchy with a virtual constructor gets value semantics. Unlike
// PayOffBridge it may be empty (default-constructed or moved-from).

#ifndef CPP4Q_JOSHI_CH05_STRATEGIES_STATISTICS_WRAPPER_H_
#define CPP4Q_JOSHI_CH05_STRATEGIES_STATISTICS_WRAPPER_H_

#include <concepts>
#include <memory>
#include <utility>

namespace cpp4q::joshi {

template <class T>
concept HasClone = requires(const T& t) {
  { t.Clone() } -> std::convertible_to<std::unique_ptr<T>>;
};

template <HasClone T>
class Wrapper {
 public:
  Wrapper() = default;
  // Clones `inner`; implicit so a concrete object can be passed directly.
  Wrapper(const T& inner) : inner_(inner.Clone()) {}  // NOLINT
  explicit Wrapper(std::unique_ptr<T> inner) : inner_(std::move(inner)) {}

  Wrapper(const Wrapper& other)
      : inner_(other.inner_ ? std::unique_ptr<T>(other.inner_->Clone())
                            : nullptr) {}
  Wrapper& operator=(const Wrapper& other) {
    if (this != &other) {
      inner_ = other.inner_ ? std::unique_ptr<T>(other.inner_->Clone())
                            : nullptr;
    }
    return *this;
  }
  Wrapper(Wrapper&&) noexcept = default;
  Wrapper& operator=(Wrapper&&) noexcept = default;
  ~Wrapper() = default;

  explicit operator bool() const { return inner_ != nullptr; }
  T& operator*() { return *inner_; }
  const T& operator*() const { return *inner_; }
  T* operator->() { return inner_.get(); }
  const T* operator->() const { return inner_.get(); }
  T* get() { return inner_.get(); }
  const T* get() const { return inner_.get(); }

 private:
  std::unique_ptr<T> inner_;
};

}  // namespace cpp4q::joshi

#endif  // CPP4Q_JOSHI_CH05_STRATEGIES_STATISTICS_WRAPPER_H_
