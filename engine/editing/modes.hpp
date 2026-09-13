#pragma once
#include "document.hpp"
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
};
std::vector<Mode> GenerateSeries(const Series &, double minimum, double maximum,
                                 unsigned capacity);
} // namespace drumfoundry::editing
