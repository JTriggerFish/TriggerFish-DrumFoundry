#pragma once
#include "document.hpp"
#include <optional>
namespace drumfoundry::editing {
struct Mode {
  double frequency, level, turbulence{1}, allocation{1};
};
// These helpers write ordinary parameters. No hidden synthesis rule is added.
std::string ModePrefix(const Document &);
std::vector<Mode> Modes(const Document &);
void SetMode(Document &, unsigned slot, Mode);
void ReplaceModes(Document &, const std::vector<Mode> &);
unsigned InsertMode(Document &, double frequency, double level);
enum class SeriesFamily { Harmonic, Membrane };
struct Series {
  SeriesFamily family{SeriesFamily::Harmonic};
  double fundamental{55}, stretch{}, level{}, rolloff{6}, turbulence{1};
  unsigned count{16}, harmonicCore{4};
  unsigned first{1};      // First partial ordinal, not a frequency offset.
  bool truncateToRange{}; // Explicit fitting policy; the editor rejects
                          // overflow.
};
std::vector<Mode> GenerateSeries(const Series &, double minimum, double maximum,
                                 unsigned capacity);
// Deform saved frequencies in ascending rank, returning them in original order.
// Negative stretch contracts; nullopt rejects the whole out-of-range candidate.
// Invalid settings throw. No clamping or removal of individual modes occurs.
std::optional<std::vector<double>>
TransformSeries(const std::vector<double> &frequencies, double pitch,
                double stretch, unsigned harmonicCore, double minimum,
                double maximum);
} // namespace drumfoundry::editing
