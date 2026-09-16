#include "percussion_test_support.hpp"
#include "runtime/modal_edit.hpp"
#include "runtime/voice.hpp"
#include "tfdsp/percussion/stochastic_modal_field.hpp"
#include <algorithm>
#include <chrono>
#include <memory>

using namespace percussion_test;
using namespace tfdsp::percussion;
namespace {
using Field = StochasticModalField<8>;
Field::Parameters Modes() {
  Field::Parameters modes{};
  for (unsigned i = 0; i < modes.size(); ++i) {
    modes[i].frequencyHz = 130.f + 80.f * i;
    modes[i].decaySeconds = 10.f;
    modes[i].inputGain = .25f;
    modes[i].identity = i + 1;
  }
  return modes;
}
void Continuity() {
  auto modes = Modes();
  StochasticModalFieldControls controls;
  controls.driftDepthHz = 1.f;
  controls.motion = {.4f, 30.f, .5f};
  Field old, next;
  old.Prepare(48000, modes, controls, 700, 6500);
  old.ProcessExcitedPair(1, 0);
  for (unsigned i = 0; i < 1000; ++i)
    old.ProcessExcitedPair(0, 0);
  next.Prepare(48000, modes, controls, 700, 6500);
  next.RetainState(old);
  CheckNear(next.StoredEnergy(), old.StoredEnergy(), 1e-8,
            "No-op preserves energy");
  for (unsigned i = 0; i < 2000; ++i)
    CheckNear(next.ProcessExcitedPair(0, 0), old.ProcessExcitedPair(0, 0), 2e-5,
              "No-op retains phase, drift and shimmer history");
}
void ReorderingAndAllocation() {
  auto modes = Modes();
  Field old, next;
  old.Prepare(48000, modes, {}, 700, 6500);
  old.ProcessExcitedPair(1, 0);
  for (unsigned i = 0; i < 333; ++i)
    old.ProcessExcitedPair(0, 0);
  std::reverse(modes.begin(), modes.end());
  next.Prepare(48000, modes, {}, 700, 6500);
  next.RetainState(old);
  for (unsigned i = 0; i < 500; ++i)
    CheckNear(next.ProcessExcitedPair(0, 0), old.ProcessExcitedPair(0, 0), 2e-6,
              "Reordering follows identities, not indices");
  const double energy = next.StoredEnergy();
  modes[3].inputGain = modes[4].inputGain = 0;
  old.Prepare(48000, modes, {}, 700, 6500);
  old.RetainState(next);
  CheckNear(old.StoredEnergy(), energy, 1e-6,
            "Removing sidebands preserves packet energy");
  while (!old.CanAdoptModalEdit())
    old.ProcessExcitedPair(0, 0);
  const double afterFade = old.StoredEnergy();
  modes[3].inputGain = modes[4].inputGain = .25f;
  next.Prepare(48000, modes, {}, 700, 6500);
  next.RetainState(old);
  CheckNear(next.StoredEnergy(), afterFade, 1e-6,
            "Adding sidebands redistributes, never injects energy");
}
void RingingPitchEdit() {
  auto modes = Modes();
  for (unsigned i = 1; i < modes.size(); ++i)
    modes[i].inputGain = 0;
  modes[0].frequencyHz = 220;
  Field old, next;
  old.Prepare(48000, modes, {}, 700, 6500);
  old.ProcessExcitedPair(1, 0);
  for (unsigned i = 0; i < 1000; ++i)
    old.ProcessExcitedPair(0, 0);
  modes[0].frequencyHz = 440;
  next.Prepare(48000, modes, {}, 700, 6500);
  next.RetainState(old);
  CheckNear(next.StoredEnergy(), old.StoredEnergy(), 1e-8,
            "Pitch editing keeps quadrature energy");
  unsigned crossings = 0;
  float previous = next.ProcessExcitedPair(0, 0);
  for (unsigned i = 0; i < 4800; ++i) {
    const float sample = next.ProcessExcitedPair(0, 0);
    crossings += previous < 0 && sample >= 0;
    previous = sample;
  }
  Check(crossings >= 43 && crossings <= 45,
        "Already ringing mode changes pitch without restrike");
}
void PacketCrossingAndSilence() {
  auto modes = Modes();
  for (unsigned i = 0; i < modes.size(); ++i) {
    modes[i].packet = i / 4;
    modes[i].packetIdentity = i / 4 + 10;
  }
  Field old, next;
  StochasticModalFieldControls controls;
  controls.motion = {.5f, 24.f, .8f};
  old.Prepare(48000, modes, controls, 700, 6500);
  old.ProcessExcitedPair(1, 0);
  for (unsigned i = 0; i < 500; ++i)
    old.ProcessExcitedPair(0, 0);
  std::rotate(modes.begin(), modes.begin() + 4, modes.end());
  for (unsigned i = 0; i < modes.size(); ++i)
    modes[i].packet = i / 4;
  next.Prepare(48000, modes, controls, 700, 6500);
  next.RetainState(old);
  for (unsigned i = 0; i < 500; ++i)
    CheckNear(next.ProcessExcitedPair(0, 0), old.ProcessExcitedPair(0, 0), 2e-5,
              "Crossing packets retains their shared motion");
  old.Reset();
  next.RetainState(
      old); // Reuse prepared storage after it held a sounding tail.
  Check(next.StoredEnergy() == 0,
        "A stale prepared buffer cannot excite silence");
  modes[1].identity = modes[0].identity;
  bool rejected = false;
  try {
    next.Prepare(48000, modes, controls, 700, 6500);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  Check(rejected, "Ambiguous live identity rejected during preparation");
}
void FreshRenderParity(const char *recipe, const char *key, double value) {
  using namespace drumfoundry;
  auto document = WithFitEnvelope(DefaultPatch(recipe));
  Voice edited(48000, document);
  auto next = edited.Document();
  for (auto &node : next["instrument"]["nodes"])
    if (node["parameters"].contains(key))
      node["parameters"][key] = value;
  auto prepared = PrepareModalEdit(48000, next);
  edited.ApplyModalEdit(*prepared);
  std::array<float, 512> a{}, b{};
  edited.Process(a.data(), a.size()); // Settle the observation ramp in silence.
  edited.Reset();
  Voice fresh(48000, next);
  double energy = 0, residual = 0;
  for (unsigned block = 0; block < 100; ++block) {
    if (block % 25 == 0) {
      Strike hit;
      hit.strength = .3f + .006f * block;
      edited.Trigger(hit);
      fresh.Trigger(hit);
    }
    edited.Process(a.data(), a.size());
    fresh.Process(b.data(), b.size());
    for (unsigned i = 0; i < a.size(); ++i) {
      energy += double(b[i]) * b[i];
      residual += double(a[i] - b[i]) * (a[i] - b[i]);
    }
  }
  Check(energy > 1e-12 && residual < energy * 1e-9,
        "Edited/reset repeated hits match freshly loaded saved parameters");
}
void RuntimeEdits() {
  using namespace drumfoundry;
  auto document = WithFitEnvelope(DefaultPatch("metal.cymbal.v1"));
  Voice voice(48000, document);
  auto values = [&](Json &d, const char *key, double value) {
    for (auto &node : d["instrument"]["nodes"])
      if (node["parameters"].contains(key))
        node["parameters"][key] = value;
  };
  auto next = voice.Document();
  values(next, "body_tune", 1.25);
  values(next, "field_packet_spread", 3.);
  auto edit = PrepareModalEdit(48000, next);
  std::array<float, 512> pcm{};
  voice.Trigger({});
  voice.Process(pcm.data(), pcm.size());
  voice.ApplyModalEdit(*edit);
  voice.Process(pcm.data(), pcm.size());
  double energy = 0;
  for (float v : pcm) {
    Check(std::isfinite(v), "Live render finite");
    energy += v * v;
  }
  Check(energy > 1e-9, "Runtime edit does not silence a ringing voice");
  // Preparation and adoption costs measured separately; never benchmark DSP
  // while building an offline comparison or allocating a replacement voice.
  auto start = std::chrono::steady_clock::now();
  for (unsigned i = 0; i < 100; ++i)
    voice.ApplyModalEdit(*edit);
  const auto elapsed = std::chrono::duration<double, std::micro>(
                           std::chrono::steady_clock::now() - start)
                           .count() /
                       100;
  std::cout << "Prepared 512-state modal update: " << elapsed << " us\n";
}
} // namespace
int main() {
  Continuity();
  ReorderingAndAllocation();
  RingingPitchEdit();
  PacketCrossingAndSilence();
  FreshRenderParity("metal.cymbal.v1", "field_packet_spread", 3.);
  FreshRenderParity("drum.kick.v1", "resonance_frequency_0", 147.);
  FreshRenderParity("drum.snare.v1", "fundamental_hz", 147.);
  RuntimeEdits();
  return failures ? 1 : 0;
}
