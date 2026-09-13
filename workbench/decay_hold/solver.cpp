#include "coordinates.hpp"
#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <map>
namespace drumfoundry::decay_hold {
namespace {
using Vector = std::vector<double>;
struct Trial {
  Vector x, residual;
  Errors errors;
  double score{};
};
std::vector<std::pair<std::string, double>>
Values(const std::vector<Axis> &axes, const Vector &x) {
  std::vector<std::pair<std::string, double>> result;
  for (unsigned i = 0; i < axes.size(); ++i)
    result.emplace_back(axes[i].key, axes[i].Decode(x[i]));
  return result;
}
Vector Residual(const Errors &errors, const std::vector<Axis> &axes,
                const Vector &x) {
  Vector r;
  for (double v : errors.late)
    r.push_back(v / std::sqrt(double(errors.late.size())));
  for (double v : errors.front)
    r.push_back(3 * std::copysign(std::max(0., std::abs(v) - 1), v) /
                std::sqrt(double(errors.front.size())));
  for (unsigned i = 0; i < axes.size(); ++i)
    r.push_back(.03 * (x[i] - axes[i].origin) / axes[i].maxStep);
  return r;
}
using Evaluate = std::function<Trial(const Vector &)>;
Vector Step(const Trial &best, const std::vector<Axis> &axes,
            const Evaluate &evaluate) {
  Eigen::MatrixXd jacobian(best.residual.size(), axes.size());
  for (unsigned i = 0; i < axes.size(); ++i) {
    auto a = best.x, b = best.x;
    a[i] = std::max(axes[i].lo, a[i] - axes[i].step);
    b[i] = std::min(axes[i].hi, b[i] + axes[i].step);
    const auto minus = evaluate(a), plus = evaluate(b);
    for (unsigned j = 0; j < best.residual.size(); ++j)
      jacobian(j, i) = (plus.residual[j] - minus.residual[j]) / (b[i] - a[i]);
  }
  Eigen::MatrixXd normal = jacobian.transpose() * jacobian;
  normal.diagonal().array() +=
      std::max(1e-5, normal.diagonal().maxCoeff() * 1e-4);
  const Eigen::Map<const Eigen::VectorXd> residual(best.residual.data(),
                                                   best.residual.size());
  const Eigen::VectorXd step =
      normal.ldlt().solve(-jacobian.transpose() * residual);
  if (!step.allFinite())
    throw std::runtime_error("Nonfinite hold-decay solver step");
  Vector result(axes.size());
  for (unsigned i = 0; i < axes.size(); ++i)
    result[i] = std::clamp(step[i], -axes[i].maxStep, axes[i].maxStep);
  return result;
}
double MaxAbs(const Vector &v) {
  double result = 0;
  for (double x : v)
    result = std::max(result, std::abs(x));
  return result;
}
bool ValidateSeed(const Document &baseline, const Document &edited,
                  const std::vector<Axis> &axes, const Trial &best,
                  uint32_t seed, const MeasureDocument &probe) {
  auto corrected = edited;
  corrected.SetMany(Values(axes, best.x));
  const uint32_t other = seed + uint32_t(911);
  const auto b = probe(baseline, other), e = probe(edited, other),
             c = probe(corrected, other);
  const Targets validation(b, e);
  const auto old = validation.Compare(e), next = validation.Compare(c);
  return Rms(next.late) <= std::max(.2, Rms(old.late)) &&
         Rms(next.late) <= 1.75 && Rms(next.front) <= 1.75;
}
Trial Improve(Trial best, const std::vector<Axis> &axes,
              const Evaluate &evaluate) {
  for (unsigned iteration = 0; iteration < 3; ++iteration) {
    const auto step = Step(best, axes, evaluate);
    bool improved = false;
    for (double scale : {1., .5, .25}) {
      auto x = best.x;
      for (unsigned i = 0; i < axes.size(); ++i)
        x[i] = std::clamp(x[i] + scale * step[i], axes[i].lo, axes[i].hi);
      const auto trial = evaluate(x);
      if (trial.score < best.score - 1e-5) {
        best = trial;
        improved = true;
        break;
      }
    }
    if (!improved)
      break;
  }
  return best;
}
} // namespace
Result Compensate(const Document &baseline, const Document &edited,
                  uint32_t seed, const MeasureDocument &measure,
                  const Cancel &cancel,
                  const std::function<void(unsigned)> &progress) {
  Result result;
  const auto probe = [&](const Document &d, uint32_t s) {
    CheckCancelled(cancel);
    if (progress)
      progress(result.evaluations + 1);
    ++result.evaluations;
    auto cells = measure(d, s);
    CheckCancelled(cancel);
    return cells;
  };
  const auto axes = Coordinates(baseline, edited);
  if (axes.empty())
    throw std::invalid_argument("No T60 controls available to hold");
  // Sequence explicitly: reproducible progress and callback ordering.
  const auto initial = probe(baseline, seed), changed = probe(edited, seed);
  const Targets target(initial, changed);
  Vector origin;
  for (const auto &a : axes)
    origin.push_back(a.origin);
  std::map<Vector, Trial> cache;
  const Evaluate evaluate = [&](const Vector &x) {
    if (auto found = cache.find(x); found != cache.end())
      return found->second;
    auto document = edited;
    document.SetMany(Values(axes, x));
    Trial trial{x, {}, target.Compare(probe(document, seed)), 0};
    trial.residual = Residual(trial.errors, axes, x);
    for (double v : trial.residual)
      trial.score += v * v;
    cache.emplace(x, trial);
    return trial;
  };
  auto best = evaluate(origin);
  result.before = result.after = Rms(best.errors.late);
  if (result.before < .15) {
    result.reason = "Decay already stable";
    return result;
  }
  best = Improve(std::move(best), axes, evaluate);
  result.after = Rms(best.errors.late);
  result.front = Rms(best.errors.front);
  for (unsigned i = 0; i < axes.size(); ++i)
    result.atLimit |=
        std::min(best.x[i] - axes[i].lo, axes[i].hi - best.x[i]) < .002;
  result.accepted = result.after < result.before * .9 && result.after <= 1.5 &&
                    result.front <= 1.5 && MaxAbs(best.errors.front) <= 4.5;
  if (result.accepted)
    result.accepted = ValidateSeed(baseline, edited, axes, best, seed, probe);
  if (result.accepted)
    result.values = Values(axes, best.x);
  result.reason =
      result.accepted ? (result.after > .5 ? "Decay partly held" : "Decay held")
      : result.atLimit ? "Cannot hold decay within control limits"
                       : "Correction rejected: tail or bloom changed too much";
  return result;
}
} // namespace drumfoundry::decay_hold
