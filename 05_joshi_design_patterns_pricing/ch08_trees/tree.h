// Chapter 8: Trees.
//
// The same separation as the exotics engine, for backward induction. A
// TreeProduct says what it pays at its final time and how it turns a
// discounted continuation value into a node value before that (a European
// passes it through, an American takes the max with exercise). The tree
// knows only about the model: SimpleBinomialTree is the book's tree in
// log-space with equal up/down probabilities, drift r - d - vol^2/2, and
// time-dependent r and d through their integrals, so every product prices
// on one tree.

#ifndef CPP4Q_JOSHI_CH08_TREES_TREE_H_
#define CPP4Q_JOSHI_CH08_TREES_TREE_H_

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

#include "ch04_virtual_constructor/parameters.h"
#include "ch04_virtual_constructor/payoff_bridge.h"

namespace cpp4q::joshi::ch08 {

class TreeProduct {
 public:
  explicit TreeProduct(double final_time) : final_time_(final_time) {}
  virtual ~TreeProduct() = default;

  double final_time() const { return final_time_; }
  virtual double FinalPayOff(double spot) const = 0;
  virtual double PreFinalValue(double spot, double time,
                               double discounted_future_value) const = 0;
  virtual std::unique_ptr<TreeProduct> Clone() const = 0;

  // A whole layer at once: on entry values[k] is the discounted future
  // value at spots[k], on exit the node value. `scratch` has the size of
  // `spots`. One virtual call per layer instead of one per node; the
  // default loops over PreFinalValue.
  virtual void PreFinalValues(std::span<const double> spots, double time,
                              std::span<double> values,
                              std::span<double> scratch) const;
  // values[k] = FinalPayOff(spots[k]).
  virtual void FinalPayOffs(std::span<const double> spots,
                            std::span<double> values) const;

 protected:
  TreeProduct(const TreeProduct&) = default;
  TreeProduct& operator=(const TreeProduct&) = default;

 private:
  double final_time_;
};

class TreeEuropean final : public TreeProduct {
 public:
  TreeEuropean(double final_time, ch04::PayOffBridge payoff)
      : TreeProduct(final_time), payoff_(std::move(payoff)) {}
  double FinalPayOff(double spot) const override { return payoff_(spot); }
  double PreFinalValue(double spot, double time,
                       double discounted_future_value) const override;
  std::unique_ptr<TreeProduct> Clone() const override;
  void PreFinalValues(std::span<const double> spots, double time,
                      std::span<double> values,
                      std::span<double> scratch) const override;
  void FinalPayOffs(std::span<const double> spots,
                    std::span<double> values) const override;

 private:
  ch04::PayOffBridge payoff_;
};

class TreeAmerican final : public TreeProduct {
 public:
  TreeAmerican(double final_time, ch04::PayOffBridge payoff)
      : TreeProduct(final_time), payoff_(std::move(payoff)) {}
  double FinalPayOff(double spot) const override { return payoff_(spot); }
  double PreFinalValue(double spot, double time,
                       double discounted_future_value) const override;
  std::unique_ptr<TreeProduct> Clone() const override;
  // Exercise values for the layer by one batched pay-off call
  // (ch04::PayOff::Evaluate), then an element-wise max.
  void PreFinalValues(std::span<const double> spots, double time,
                      std::span<double> values,
                      std::span<double> scratch) const override;
  void FinalPayOffs(std::span<const double> spots,
                    std::span<double> values) const override;

 private:
  ch04::PayOffBridge payoff_;
};

class SimpleBinomialTree {
 public:
  // Throws unless steps > 0 and time > 0.
  SimpleBinomialTree(double spot, ch04::Parameters r, ch04::Parameters d,
                     double vol, std::size_t steps, double time);

  // Prices `product`, whose final time must equal the tree's time.
  double GetThePrice(const TreeProduct& product);

 private:
  void BuildTree();

  double spot_;
  ch04::Parameters r_;
  ch04::Parameters d_;
  double vol_;
  std::size_t steps_;
  double time_;
  bool tree_built_ = false;
  // The spot at step i, node k (k up-moves) is lowest_spot_[i] * up_pow_[k]
  // with up_pow_[k] = exp(2 vol sqrt(dt))^k, so the tree is stored in
  // O(steps) memory and needs no exp per node. (The book stores every
  // node's spot and value: O(steps^2) memory and one exp per node; see
  // profiling/REPORT.md.)
  std::vector<double> lowest_spot_;
  std::vector<double> up_pow_;
  std::vector<double> discounts_;  // one-step discount factor per step
  // One layer each of spots, option values and product scratch space.
  std::vector<double> spots_;
  std::vector<double> values_;
  std::vector<double> scratch_;
};

}  // namespace cpp4q::joshi::ch08

#endif  // CPP4Q_JOSHI_CH08_TREES_TREE_H_
