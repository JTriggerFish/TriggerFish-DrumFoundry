#include "percussion_test_support.hpp"
#include "tfdsp/percussion/stochastic_modal_field.hpp"

using namespace percussion_test;
using Field = tfdsp::percussion::StochasticModalField<8>;
namespace {
Field::Parameters Modes() {
  Field::Parameters modes{};
  for (unsigned i = 0; i < modes.size(); ++i) {
    modes[i].frequencyHz = 20.f + i;
    modes[i].decaySeconds = 10;
    modes[i].inputGain = .25f;
    modes[i].identity = i + 1;
  }
  return modes;
}
void Removal(float rate, unsigned remaining) {
  auto modes = Modes();
  Field old, next, pending;
  old.Prepare(rate, modes, {}, 700, 6500);
  old.ProcessExcitedPair(1, 0);
  for (unsigned i = remaining; i < modes.size(); ++i)
    modes[i].inputGain = 0;
  next.Prepare(rate, modes, {}, 700, 6500);
  Check(next.RetainState(old), "First removal accepted");
  if (remaining)
    CheckNear(next.StoredEnergy(), old.StoredEnergy(), 1e-6,
              "Retirement does not duplicate energy in the resonant body");
  else
    Check(next.StoredEnergy() == 0, "Deleted packet leaves no body energy");
  Check(!next.CanAdoptModalEdit(), "Removed modes must finish their fade");
  const double expected = old.ProcessExcitedPair(0, 0);
  double previous = next.ProcessExcitedPair(0, 0);
  CheckNear(previous, expected, .012,
            "Removal starts continuously, not with the old 25% step");
  // Repeated edits are rejected without mutating either prepared or sounding
  // state. A latest-target mailbox retries once the fixed storage is free.
  pending.Prepare(rate, Modes(), {}, 700, 6500);
  Check(!pending.RetainState(next), "Overlapping removal must be deferred");
  Check(pending.StoredEnergy() == 0, "Deferred destination untouched");
  const unsigned samples = std::max(1u, unsigned(.005f * rate));
  double maxStep = 0;
  for (unsigned i = 1; i < samples; ++i) {
    const double output = next.ProcessExcitedPair(0, 0);
    maxStep = std::max(maxStep, std::abs(output - previous));
    previous = output;
    if (i + 1 < samples)
      Check(!next.CanAdoptModalEdit(), "Fade completed too early");
  }
  Check(maxStep < .02, "Removal fade contains an abrupt sample step");
  Check(next.CanAdoptModalEdit(), "Removal finishes in exactly 5 ms");
  Check(pending.RetainState(next), "Deferred target accepted after fade");
  if (!remaining)
    Check(previous == 0 && next.ProcessExcitedPair(0, 0) == 0,
          "Full deletion leaves no residual output after fade");
  next.Reset();
  Check(next.CanAdoptModalEdit() && next.ProcessExcitedPair(0, 0) == 0,
        "Reset clears all retirement state");
}
void MuteAndReset() {
  auto modes = Modes();
  Field old, next;
  old.Prepare(48000, modes, {}, 700, 6500);
  old.ProcessExcitedPair(1, 0);
  for (auto &m : modes)
    m.inputGain = 0;
  next.Prepare(48000, modes, {}, 700, 6500);
  next.RetainState(old);
  Check(next.ProcessExcitedPair(0, 0, {0, 1, 1, 1}) == 0,
        "Retiring output obeys mute damping");
  next.Reset();
  Check(next.CanAdoptModalEdit() && next.ProcessExcitedPair(0, 0) == 0,
        "Panic clears retirement before the fade completes");
}
} // namespace
int main() {
  for (float rate : {44100.f, 48000.f, 96000.f})
    for (unsigned remaining : {0u, 6u})
      Removal(rate, remaining);
  MuteAndReset();
  return failures ? 1 : 0;
}
