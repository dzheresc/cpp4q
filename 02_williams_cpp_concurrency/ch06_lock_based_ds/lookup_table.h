// Chapter 6: a thread-safe lookup table with per-bucket locks.
//
// A fixed number of buckets, each a small list guarded by its own
// std::shared_mutex. Operations on different symbols almost never touch
// the same lock, and readers of one bucket run in parallel. Used here as a
// position book keyed by symbol.

#ifndef CPP4Q_WILLIAMS_CH06_LOCK_BASED_DS_LOOKUP_TABLE_H_
#define CPP4Q_WILLIAMS_CH06_LOCK_BASED_DS_LOOKUP_TABLE_H_

#include <algorithm>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <vector>

namespace cpp4q::williams::ch06 {

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class ThreadSafeLookupTable {
 public:
  explicit ThreadSafeLookupTable(std::size_t num_buckets = 31, Hash hasher = {})
      : buckets_(std::max<std::size_t>(num_buckets, 1)),
        hasher_(std::move(hasher)) {
    for (auto& b : buckets_) b = std::make_unique<Bucket>();
  }
  ThreadSafeLookupTable(const ThreadSafeLookupTable&) = delete;
  ThreadSafeLookupTable& operator=(const ThreadSafeLookupTable&) = delete;

  [[nodiscard]] Value ValueFor(const Key& key, Value fallback = {}) const {
    const Bucket& b = BucketFor(key);
    std::shared_lock lock(b.mutex);
    const auto it = b.Find(key);
    return it == b.items.end() ? fallback : it->second;
  }

  void AddOrUpdate(const Key& key, const Value& value) {
    Bucket& b = BucketFor(key);
    std::unique_lock lock(b.mutex);
    if (auto it = b.Find(key); it != b.items.end()) {
      it->second = value;
    } else {
      b.items.emplace_back(key, value);
    }
  }

  // Applies `update(value&)` to the entry for `key` (default-constructed if
  // missing) under the bucket's exclusive lock: an atomic read-modify-write.
  template <typename F>
  void Modify(const Key& key, F&& update) {
    Bucket& b = BucketFor(key);
    std::unique_lock lock(b.mutex);
    auto it = b.Find(key);
    if (it == b.items.end()) {
      b.items.emplace_back(key, Value{});
      it = std::prev(b.items.end());
    }
    std::forward<F>(update)(it->second);
  }

  bool Remove(const Key& key) {
    Bucket& b = BucketFor(key);
    std::unique_lock lock(b.mutex);
    if (auto it = b.Find(key); it != b.items.end()) {
      b.items.erase(it);
      return true;
    }
    return false;
  }

  // A consistent snapshot: every bucket is locked (in index order, so no
  // deadlock with another Snapshot) before any is read.
  [[nodiscard]] std::map<Key, Value> Snapshot() const {
    std::vector<std::shared_lock<std::shared_mutex>> locks;
    locks.reserve(buckets_.size());
    for (const auto& b : buckets_) locks.emplace_back(b->mutex);
    std::map<Key, Value> out;
    for (const auto& b : buckets_) {
      for (const auto& [k, v] : b->items) out.emplace(k, v);
    }
    return out;
  }

 private:
  struct Bucket {
    using Items = std::list<std::pair<Key, Value>>;
    auto Find(const Key& key) {
      return std::find_if(items.begin(), items.end(),
                          [&](const auto& kv) { return kv.first == key; });
    }
    auto Find(const Key& key) const {
      return std::find_if(items.begin(), items.end(),
                          [&](const auto& kv) { return kv.first == key; });
    }
    Items items;
    mutable std::shared_mutex mutex;
  };

  Bucket& BucketFor(const Key& key) const {
    return *buckets_[hasher_(key) % buckets_.size()];
  }

  // The vector itself never changes after construction, so it needs no lock.
  std::vector<std::unique_ptr<Bucket>> buckets_;
  Hash hasher_;
};

}  // namespace cpp4q::williams::ch06

#endif  // CPP4Q_WILLIAMS_CH06_LOCK_BASED_DS_LOOKUP_TABLE_H_
