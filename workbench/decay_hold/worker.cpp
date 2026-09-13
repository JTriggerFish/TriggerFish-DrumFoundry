#include "worker.hpp"
#include "runtime/voice.hpp"
namespace drumfoundry::decay_hold {
Worker::~Worker() {
  Cancel();
  if (future_.valid())
    future_.wait();
}
void Worker::Cancel() { cancel_ = true; }
bool Worker::Start(editing::Json baseline, editing::Json edited,
                   unsigned rate) {
  if (busy_)
    return false;
  if (future_.valid())
    future_.get();
  cancel_ = false;
  evaluations_ = 0;
  busy_ = true;
  try {
    future_ =
        std::async(std::launch::async, [this, baseline = std::move(baseline),
                                        edited = std::move(edited), rate] {
          Completion completion;
          try {
            const decay_hold::Cancel cancelled = [this] {
              return cancel_.load();
            };
            CheckCancelled(cancelled);
            Document before, after;
            before.Load(baseline);
            after.Load(edited);
            CheckCancelled(cancelled);
            if (!Eligible(before, after)) {
              busy_ = false;
              return completion;
            }
            Voice voice(float(rate), edited);
            completion.result = Compensate(
                before, after, voice.Event().seed,
                [&](const Document &d, uint32_t seed) {
                  return RenderMeasure(d, seed, rate, cancelled);
                },
                cancelled, [this](unsigned n) { evaluations_ = n; });
          } catch (const std::exception &e) {
            completion.error = e.what();
          }
          busy_ = false;
          return completion;
        });
  } catch (...) {
    busy_ = false;
    throw;
  }
  return true;
}
std::optional<Completion> Worker::Take() {
  if (!future_.valid() ||
      future_.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
    return {};
  auto result = future_.get();
  if (cancel_)
    return {};
  return result;
}
} // namespace drumfoundry::decay_hold
