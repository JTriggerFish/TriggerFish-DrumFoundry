#include "percussion_test_support.hpp"
#include "tfdsp/percussion/membrane_resonator.hpp"
using namespace tfdsp::percussion;
using percussion_test::Check;

void Exercise(float rate, float tension, float openness) {
  using Body = MembraneResonator<12>;
  Body::Parameters modes{};
  for (unsigned i = 0; i < modes.size(); ++i)
    modes[i] = {90.f + 211.f * i, 30.f, 1.f, 1.f, 1.f, 1.f};
  Body body, freeBody;
  body.Prepare(rate, modes);
  freeBody.Prepare(rate, modes);
  ModalRimContactParameters rim{true, openness, .0001f, .5f, 1.f, .7f, .8f};
  body.SetRimContact(rim, true);
  Body::Drive silent{};
  Check(body.Process(silent, tension) == 0 && body.StoredEnergy() == 0,
        "Loading contact on a membrane is silent");
  body.Process(body.Project(1, .5f), tension);
  freeBody.Process(freeBody.Project(1, .5f), tension);
  const float initial = body.StoredEnergy();
  for (int i = 0; i < int(rate / 2); ++i) {
    const auto before = body.StoredEnergy();
    const auto sample = body.Process(silent, tension);
    freeBody.Process(silent, tension);
    Check(std::isfinite(sample), "Membrane with contact remains finite");
    Check(body.StoredEnergy() <= before + 1.e-6f * initial,
          "Stationary membrane contact cannot add energy, including rattle storage");
    if (i == 1024) {
      const auto energy = body.StoredEnergy();
      modes[3].frequencyHz *= 1.1f;
      body.SetPreparedParameters(Body::PrepareParameters(rate, modes));
      freeBody.SetPreparedParameters(Body::PrepareParameters(rate, modes));
      Check(body.StoredEnergy() == energy, "Modal edit retains membrane and contact energy");
    }
  }
  // Open contact may hold energy in its bouncing mass, outside modal T60.
  // Only the closed, fixed-boundary case must decay faster than a free body.
  if (openness == 0)
    Check(body.StoredEnergy() < freeBody.StoredEnergy() * .95f,
          "Closed contact dissipates more energy than modal damping alone");
  body.Reset();
  Check(body.StoredEnergy() == 0, "Reset clears both membrane and attached mass");
  rim.openness = 1;
  body.SetRimContact(rim, true);
  rim.openness = 0;
  body.SetRimContact(rim);
  float maximum = 0;
  for (int i = 0; i < int(rate / 10); ++i) {
    body.Process(silent, tension);
    maximum = std::max(maximum, body.StoredEnergy());
  }
  Check(maximum > 0, "Live closure supplies explicit actuator work on membrane");
}
int main() {
  for (float rate : {44100.f, 48000.f, 96000.f})
    for (float tension : {.5f, 1.f, 2.f})
      for (float openness : {0.f, .35f}) Exercise(rate, tension, openness);
  return percussion_test::failures ? 1 : 0;
}
