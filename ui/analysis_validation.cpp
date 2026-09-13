#include "analysis_validation.hpp"
#include <cmath>
#include <stdexcept>
namespace drumfoundry::ui {
namespace {
double Number(const editing::Json &v, const char *key, double fallback,
              double low, double high) {
  const double value = v.value(key, fallback);
  if (!std::isfinite(value) || value < low || value > high)
    throw std::invalid_argument(std::string("Invalid saved analysis field: ") +
                                key);
  return value;
}
} // namespace
editing::Json ReadSavedView(const editing::Json &analysis) {
  if (!analysis.contains("view"))
    return nullptr;
  const auto &v = analysis.at("view");
  struct Field {
    const char *key;
    double initial, low, high;
  };
  static const Field fields[] = {
      {"comparison", 0, 0, 5},         {"span", 8, .02, 60},
      {"pan", 0, -1e6, 1e6},           {"split", .5, .1, .9},
      {"modelOffset", 0, -1e6, 1e6},   {"differenceDb", 24, 1, 120},
      {"frequencyLow", 20, 20, 19999}, {"frequencyHigh", 20000, 20, 20000},
      {"renderSeconds", 8, .25, 60},   {"analysisShare", 450. / 1100, .1, .9}};
  auto result = editing::Json::object();
  for (auto f : fields)
    result[f.key] = Number(v, f.key, f.initial, f.low, f.high);
  const double mode = result.at("comparison");
  if (mode != std::floor(mode) || result.at("frequencyHigh").get<double>() <=
                                      result.at("frequencyLow").get<double>())
    throw std::invalid_argument("Invalid saved comparison or frequency zoom");
  return result;
}
void ValidateAnalysisDocument(const editing::Json &document) {
  const auto &analysis = document.at("controls").at("analysis");
  ReadSavedView(analysis);
  const double size = Number(analysis, "size", 4096, 64, 32768),
               hop = Number(analysis, "hop", 1024, 1, size);
  if (size != std::floor(size) || (unsigned(size) & (unsigned(size) - 1)) ||
      hop != std::floor(hop))
    throw std::invalid_argument("Invalid saved FFT size or hop");
  const auto window = analysis.at("window").get<std::string>();
  if (window != "hann" && window != "blackman-harris" &&
      window != "rectangular")
    throw std::invalid_argument("Invalid saved FFT window");
  Number(analysis, "dynamicRangeDb", 80, 30, 120);
  const auto &ref = document.at("reference");
  if (ref.is_null())
    return;
  for (const char *key : {"id", "sha256", "name", "localPath"})
    if (ref.contains(key) && !ref.at(key).is_string())
      throw std::invalid_argument(std::string("Invalid reference ") + key);
  const double channel = Number(ref, "channel", 0, 0, 2);
  if (channel != std::floor(channel))
    throw std::invalid_argument("Invalid reference channel");
  Number(ref, "referenceGainDb", 0, -60, 48);
  Number(ref, "duration", 8, 0, 60);
  if (ref.contains("cell"))
    Number(ref.at("cell"), "onset_seconds", 0, -1e6, 1e6);
}
} // namespace drumfoundry::ui
