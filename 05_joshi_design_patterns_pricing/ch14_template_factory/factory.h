// Chapter 14: Templatizing the factory.
//
// Chapter 10's factory was written for pay-offs constructed from a strike.
// Everything in it except those two types is generic, so it becomes a
// template: Factory<Base, Args...> maps names to creators of Base from
// Args..., and Registrar<Base, Derived, Args...> registers Derived. Each
// (Base, Args...) pair gets its own registry, so pay-offs from a strike,
// pay-offs from two barriers and random generators from (dimension, seed)
// all have factories without another line of factory code.
//
// Changes from the book's version: creators are std::function (so lambdas
// with captures register too), arguments are forwarded, ids can be removed,
// and the registry is guarded by a std::shared_mutex, so lookups from many
// threads run concurrently and late registration is safe.

#ifndef CPP4Q_JOSHI_CH14_TEMPLATE_FACTORY_FACTORY_H_
#define CPP4Q_JOSHI_CH14_TEMPLATE_FACTORY_FACTORY_H_

#include <concepts>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cpp4q::joshi::ch14 {

template <class Base, class... Args>
class Factory {
 public:
  using Creator = std::function<std::unique_ptr<Base>(Args...)>;

  // The program-wide registry for this (Base, Args...).
  static Factory& Instance() {
    static Factory the_factory;
    return the_factory;
  }

  Factory() = default;
  Factory(const Factory&) = delete;
  Factory& operator=(const Factory&) = delete;

  // Returns false (keeping the existing creator) if `id` is taken.
  bool Register(std::string id, Creator creator) {
    if (!creator) throw std::invalid_argument("Factory: empty creator");
    std::unique_lock lock(mutex_);
    return creators_.emplace(std::move(id), std::move(creator)).second;
  }

  bool Unregister(std::string_view id) {
    std::unique_lock lock(mutex_);
    const auto it = creators_.find(id);
    if (it == creators_.end()) return false;
    creators_.erase(it);
    return true;
  }

  bool Contains(std::string_view id) const {
    std::shared_lock lock(mutex_);
    return creators_.find(id) != creators_.end();
  }

  // Throws std::out_of_range if `id` is unknown. The creator runs outside
  // the lock, so it may itself use the factory.
  std::unique_ptr<Base> Create(std::string_view id, Args... args) const {
    Creator creator;
    {
      std::shared_lock lock(mutex_);
      const auto it = creators_.find(id);
      if (it == creators_.end()) {
        throw std::out_of_range("Factory: unknown id '" + std::string(id) +
                                "'");
      }
      creator = it->second;
    }
    return creator(std::forward<Args>(args)...);
  }

  std::vector<std::string> Keys() const {
    std::shared_lock lock(mutex_);
    std::vector<std::string> keys;
    keys.reserve(creators_.size());
    for (const auto& [id, creator] : creators_) keys.push_back(id);
    return keys;
  }

 private:
  mutable std::shared_mutex mutex_;
  std::map<std::string, Creator, std::less<>> creators_;
};

// Registers Derived (constructible from Args...) in the global
// Factory<Base, Args...> when constructed.
template <class Base, class Derived, class... Args>
  requires std::derived_from<Derived, Base> &&
           std::constructible_from<Derived, Args...>
class Registrar {
 public:
  explicit Registrar(std::string id) {
    Factory<Base, Args...>::Instance().Register(
        std::move(id), [](Args... args) -> std::unique_ptr<Base> {
          return std::make_unique<Derived>(std::forward<Args>(args)...);
        });
  }
};

}  // namespace cpp4q::joshi::ch14

#endif  // CPP4Q_JOSHI_CH14_TEMPLATE_FACTORY_FACTORY_H_
