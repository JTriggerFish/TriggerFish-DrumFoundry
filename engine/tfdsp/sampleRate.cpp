#include "sampleRate.hpp"

namespace tfdsp {

std::unique_ptr<X2Resampler_Order7> CreateX2Resampler_Chebychev7() {
  Eigen::Array<double, 2, 1> directCoeffs;
  Eigen::Array<double, 1, 1> delayedCoeffs;
  directCoeffs << 0.081430023176616115, 0.70977080010248506;
  delayedCoeffs << 0.31565984021666094;
  return std::make_unique<X2Resampler_Order7>(directCoeffs, delayedCoeffs);
}
std::unique_ptr<DummyResampler> CreateDummyResampler() {
  return std::make_unique<DummyResampler>();
}
std::unique_ptr<X4Resampler_Order7> CreateX4Resampler_Cheby7() {
  return std::make_unique<X4Resampler_Order7>(CreateX2Resampler_Chebychev7);
}

} // namespace tfdsp
