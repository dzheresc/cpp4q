#include "ch04_synchronizing/futures.h"

#include <barrier>
#include <cmath>
#include <latch>
#include <random>
#include <stdexcept>

namespace cpp4q::williams::ch04 {

PricingService::PricingService()
    : worker_([this] {
        while (auto job = jobs_.WaitAndPop()) (*job)();
      }) {}

PricingService::~PricingService() { jobs_.Close(); }

std::future<double> PricingService::Submit(std::function<double()> job) {
  std::packaged_task<double()> task(std::move(job));
  auto result = task.get_future();
  if (!jobs_.Push(std::move(task))) {
    throw std::runtime_error("PricingService: shutting down");
  }
  return result;
}

std::future<double> HistoricalVarAsync(std::vector<double> pnl,
                                       double confidence) {
  std::promise<double> promise;
  auto result = promise.get_future();
  std::thread([p = std::move(promise), pnl = std::move(pnl),
               confidence]() mutable {
    try {
      if (pnl.empty() || !(confidence > 0.0 && confidence < 1.0)) {
        throw std::invalid_argument("HistoricalVarAsync: bad input");
      }
      const auto k = static_cast<std::size_t>(
          std::floor((1.0 - confidence) * static_cast<double>(pnl.size())));
      std::nth_element(pnl.begin(), pnl.begin() + static_cast<long>(k),
                       pnl.end());
      p.set_value(-pnl[k]);
    } catch (...) {
      p.set_exception(std::current_exception());
    }
  }).detach();  // the promise/future pair is the only link back
  return result;
}

std::vector<double> LockstepMeanSpot(std::size_t num_paths,
                                     std::size_t num_steps, double spot,
                                     double vol, double dt,
                                     unsigned num_workers, std::uint64_t seed) {
  if (num_workers == 0) num_workers = 1;
  std::vector<double> spots(num_paths, spot);
  std::vector<double> mean_by_step;
  mean_by_step.reserve(num_steps);
  const double drift = -0.5 * vol * vol * dt;
  const double diffusion = vol * std::sqrt(dt);

  // Runs on one thread after all workers arrive; workers are blocked, so it
  // may read `spots` without a data race.
  // Each worker sums its own slice into its own cache line, so the
  // completion step adds num_workers numbers instead of re-reading every
  // path serially while all workers wait (profiling/REPORT.md, finding 4).
  struct alignas(64) PartialSum {
    double value = 0.0;
  };
  std::vector<PartialSum> partial(num_workers);
  auto on_step_done = [&]() noexcept {
    double sum = 0.0;
    for (const auto& p : partial) sum += p.value;
    mean_by_step.push_back(sum / static_cast<double>(num_paths));
  };
  std::barrier step_barrier(static_cast<std::ptrdiff_t>(num_workers),
                            on_step_done);
  std::latch start(static_cast<std::ptrdiff_t>(num_workers));

  auto worker = [&](unsigned w) {
    const std::size_t lo = num_paths * w / num_workers;
    const std::size_t hi = num_paths * (w + 1) / num_workers;
    std::seed_seq seq{seed, static_cast<std::uint64_t>(w)};
    std::mt19937_64 rng(seq);
    std::normal_distribution<double> z;
    start.arrive_and_wait();
    for (std::size_t step = 0; step < num_steps; ++step) {
      double sum = 0.0;
      for (std::size_t i = lo; i < hi; ++i) {
        spots[i] *= std::exp(drift + diffusion * z(rng));
        sum += spots[i];
      }
      partial[w].value = sum;
      step_barrier.arrive_and_wait();
    }
  };
  {
    std::vector<std::jthread> threads;
    threads.reserve(num_workers);
    for (unsigned w = 0; w < num_workers; ++w) threads.emplace_back(worker, w);
  }
  return mean_by_step;
}

}  // namespace cpp4q::williams::ch04
