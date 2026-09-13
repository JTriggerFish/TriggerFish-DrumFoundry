#include "meta.hpp"
#include "size_endpoints.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace drumfoundry::editing {
MetaEdit SizeMeta(const Document &document, double position) {
  if (!std::isfinite(position) || position < 0 || position > 1)
    throw std::invalid_argument("Size meta must be between zero and one");
  auto large = LargeSizeEndpoint(), small = SmallSizeEndpoint();
  std::vector<const Parameter *> frequencies;
  for (const auto &p : document.Parameters())
    if (p.key.rfind("resolved_frequency_", 0) == 0)
      frequencies.push_back(&p);
  std::sort(frequencies.begin(), frequencies.end(), [](auto *a, auto *b) {
    return std::stoi(a->key.substr(19)) < std::stoi(b->key.substr(19));
  });
  if (frequencies.empty())
    throw std::invalid_argument("Size meta needs a metallic modal body");
  const double levels[]{-7, -5, -3, 0, 4, 7, 6, 5, 3, 2, 0, -2};
  for (std::size_t i = 0; i < frequencies.size(); ++i) {
    const auto *p = frequencies[i];
    const double at =
        frequencies.size() > 1 ? i * 11. / (frequencies.size() - 1) : 0;
    const int a = int(at), b = std::min(11, a + 1);
    small[p->key] = p->initial * 1.45;
    small["resolved_level_" + p->key.substr(19)] =
        levels[a] + (at - a) * (levels[b] - levels[a]);
  }
  MetaEdit edit;
  const auto &endpoint = position < .5 ? large : small;
  const double blend = position < .5 ? position * 2 : (position - .5) * 2;
  for (const auto &p : document.Parameters()) {
    if (!large.count(p.key) && !small.count(p.key))
      continue;
    const double end = endpoint.count(p.key) ? endpoint.at(p.key) : p.initial;
    const double a = position < .5 ? end : p.initial,
                 b = position < .5 ? p.initial : end;
    double value =
        p.scale == 1 ? a * std::pow(b / a, blend) : a + (b - a) * blend;
    if (p.scale >= 2)
      value = std::round(value);
    const double bounded = std::clamp(value, p.minimum, p.maximum);
    if (value != bounded)
      edit.limited.push_back(ControlName(p));
    edit.values.emplace_back(p.key, bounded);
  }
  return edit;
}
} // namespace drumfoundry::editing
