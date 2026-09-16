#include "percussion_test_support.hpp"
#include "tfdsp/percussion/wire_rack.hpp"

using namespace percussion_test;
using namespace tfdsp::percussion;
namespace {
void Excite(WireRack &rack) {
  for (unsigned i = 0; i < 4000; ++i)
    rack.Process(.2f * Sine(i, 185, 48000));
}
void NoOpAndHistory() {
  const auto p = PrepareWireRackParameters(48000, {});
  WireRack old;
  old.Prepare(p);
  Excite(old);
  WireRack next = old;
  Check(next.AdoptPrepared(p), "No-op wire update accepted");
  CheckNear(next.ContactAmount(), old.ContactAmount(), 0, "Follower retained");
  CheckNear(next.StoredEnergy(), old.StoredEnergy(), 1e-9, "Wire energy retained");
  for (unsigned i = 0; i < 4000; ++i) {
    const float input = .1f * Sine(i, 211, 48000);
    CheckNear(next.Process(input), old.Process(input), 1e-7,
              "No-op retains phase, noise RNG and colour filter history");
  }
}
void Allocation() {
  WireRackParameters controls;
  controls.density = 1;
  auto p = PrepareWireRackParameters(48000, controls);
  // Coherent low oscillators expose a deletion click clearly, without a
  // broadband stochastic signal masking the discontinuity under test.
  for (unsigned i = 0; i < WireRackModeCount; ++i) {
    p.cosine[i] = std::cos(6.28318530718f * 20 / 48000);
    p.sine[i] = std::sin(6.28318530718f * 20 / 48000);
    p.radius[i] = 1;
    p.inputPhaseCosine[i] = 1;
    p.inputPhaseSine[i] = 0;
    p.modeOutputGain[i] = 1.f / std::sqrt(float(WireRackModeCount));
  }
  p.controls.noiseMix = 0;
  p.controls.modalMix = 1;
  WireRack rack;
  rack.Prepare(p);
  Excite(rack);
  controls.sensitivity = 0;
  controls.noiseMix = 0;
  controls.modalMix = 1;
  rack.SetLiveControls(controls);
  for (unsigned i = 0; i < 512; ++i)
    rack.Process(0);
  auto unchanged = rack;
  auto small = p;
  small.activeModeCount = 8;
  small.modeOutputGain.fill(0);
  for (unsigned i = 0; i < 8; ++i)
    small.modeOutputGain[i] = 1.f / std::sqrt(8.f);
  const float energy = rack.StoredEnergy();
  Check(energy > 1e-12, "Allocation test has a ringing wire bank");
  Check(rack.AdoptPrepared(small), "Wire removal accepted");
  CheckNear(rack.StoredEnergy(), energy, energy * 1e-5, "Removal redistributes energy");
  Check(!rack.CanAdoptPrepared() && !rack.AdoptPrepared(p), "Rapid edit deferred");
  const float expected = unchanged.Process(0);
  CheckNear(rack.Process(0), expected, std::abs(expected) * .01 + 1e-9,
            "Removed wires fade instead of disappearing abruptly");
  for (unsigned i = 1; i < 240; ++i)
    rack.Process(0);
  Check(rack.CanAdoptPrepared(), "Wire fade finishes in 5 ms");
  const float remaining = rack.StoredEnergy();
  Check(rack.AdoptPrepared(p), "Wire additions accepted after fade");
  CheckNear(rack.StoredEnergy(), remaining, remaining * 1e-5,
            "New wires share energy, never create it");
  rack.Reset();
  Check(rack.StoredEnergy() == 0 && rack.CanAdoptPrepared() && rack.Process(0) == 0,
        "Reset clears ringing and retirement");
  Check(rack.AdoptPrepared(small) && rack.AdoptPrepared(p),
        "Silent geometry edits need no retirement wait");
  Check(rack.Process(0) == 0, "Silent density editing cannot synthesize a hit");
}
void SmoothMixAndCurrentTargets() {
  WireRack rack;
  WireRackParameters p;
  p.noiseMix = 0;
  rack.Prepare(48000, p);
  Excite(rack);
  auto unchanged = rack;
  p.modalMix = 0;
  rack.SetLiveControls(p);
  const float expected = unchanged.Process(0);
  CheckNear(rack.Process(0), expected * (239.f / 240.f), 1e-7,
            "Wire mix is ramped, not stepped");
  // Old geometry snapshots must not undo newer gain/brightness automation.
  auto oldSnapshot = PrepareWireRackParameters(48000, {});
  Check(rack.AdoptPrepared(oldSnapshot), "Geometry accepted with newer controls");
  for (unsigned i = 0; i < 512; ++i)
    rack.Process(.1f);
  Check(rack.Process(.1f) == 0, "Old snapshot did not restore old wire mixes");
}
} // namespace
int main() {
  NoOpAndHistory();
  Allocation();
  SmoothMixAndCurrentTargets();
  return failures ? 1 : 0;
}
