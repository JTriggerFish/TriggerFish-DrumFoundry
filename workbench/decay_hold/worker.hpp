#pragma once
#include "solver.hpp"
#include <atomic>
#include <future>
#include <optional>
namespace drumfoundry::decay_hold {
struct Completion {
  Result result;
  std::string error;
};
// One independent native solve. Cancel is nonblocking; destruction joins at a
// render-block boundary. Superseded completions are never applied to the UI.
class Worker {
public:
  ~Worker();
  bool Start(editing::Json baseline, editing::Json edited, unsigned rate);
  void Cancel();
  bool Busy() const { return busy_; }
  unsigned Evaluations() const { return evaluations_; }
  std::optional<Completion> Take();

private:
  std::future<Completion> future_;
  std::atomic<bool> cancel_{}, busy_{};
  std::atomic<unsigned> evaluations_{};
};
} // namespace drumfoundry::decay_hold
