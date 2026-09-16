#include "percussion_test_support.hpp"
#include "runtime/modal_edit.hpp"
#include "runtime/live_controls.hpp"
#include "runtime/voice.hpp"

using namespace percussion_test;
using namespace drumfoundry;
namespace {
void Set(Json &d, const std::string &key, double value) {
  for (auto &node : d["instrument"]["nodes"])
    if (node["parameters"].contains(key))
      node["parameters"][key] = value;
}
void PreparedParity(float rate) {
  auto before = WithFitEnvelope(DefaultPatch("drum.snare.v1"));
  Voice metadata(rate, before);
  unsigned covered = 0;
  for (const auto &d : metadata.Descriptors()) {
    const auto key = d.at("key").get<std::string>();
    if (key == "direct_delay_ms") {
      Check(!IsLiveParameter("drum.snare.v1", key) &&
                !IsPreparedLiveParameter("drum.snare.v1", key), "Delay remains structural");
      continue;
    }
    Check(IsLiveParameter("drum.snare.v1", key) ||
              IsPreparedLiveParameter("drum.snare.v1", key), "All other snare controls live");
    if (!IsPreparedLiveParameter("drum.snare.v1", key))
      continue;
    ++covered;
    for (double value : {d.at("minimum").get<double>(), d.at("maximum").get<double>()}) {
      auto next = before;
      Set(next, key, value);
      Check(ValidateLiveEdit(before, next, true), "Prepared snare classifier agrees");
      Voice edited(rate, before), fresh(rate, next);
      edited.Trigger({});
      std::array<float, 512> a{}, b{};
      for (unsigned i = 0; i < 4; ++i)
        edited.Process(a.data(), a.size());
      auto p = PrepareModalEdit(rate, next);
      Check(edited.ApplyModalEdit(*p), "Prepared snare edit accepted in tail");
      edited.Process(a.data(), a.size());
      for (float v : a)
        Check(std::isfinite(v), "Live snare stays finite");
      edited.Reset();
      edited.Trigger(fresh.Event());
      fresh.Trigger(fresh.Event());
      double error = 0, energy = 0;
      for (unsigned block = 0; block < 16; ++block) {
        edited.Process(a.data(), a.size());
        fresh.Process(b.data(), b.size());
        for (unsigned i = 0; i < a.size(); ++i) {
          error += std::pow(double(a[i]) - b[i], 2);
          energy += double(b[i]) * b[i];
        }
      }
      if (error > 1e-12 + energy * 1e-9)
        std::cerr << "Snare prepared parity failed: " << key << '=' << value << '\n';
      Check(error <= 1e-12 + energy * 1e-9, "Live saved sound matches fresh render");
    }
  }
  Check(covered == 10, "All ten prepared snare controls exercised");
}
} // namespace
int main() {
  for (float rate : {44100.f, 48000.f, 96000.f})
    PreparedParity(rate);
  return failures ? 1 : 0;
}
