#include "size_endpoints.hpp"
#include <cmath>
namespace drumfoundry::editing {
SizeValues LargeSizeEndpoint() {
  SizeValues values{{"model_level_db", 0},
                    {"impact_tone_noise", .2},
                    {"impact_width", 2.2},
                    {"impact_chirp_pitch", .15},
                    {"bloom_rate", .5},
                    {"bloom_energy_acceleration", .8},
                    {"bloom_energy_sensitivity", 1.6},
                    {"body_brightness", -56},
                    {"body_excitation_centre", 1100},
                    {"body_excitation", 1},
                    {"field_turbulence", .72 * std::pow(1000. / 1200, .6)},
                    {"field_turbulence_slope", .6},
                    {"field_packet_spread", 5.5},
                    {"field_satellite_density", .65},
                    {"field_phase_bandwidth", .8},
                    {"field_gain", 4},
                    {"direct_gain", .008},
                    {"output_low_cut", 25},
                    {"output_colour_frequency", 8500},
                    {"output_colour_gain", 2},
                    {"output_high_cut", 12000},
                    {"body_decay_seconds_0", 12},
                    {"body_decay_seconds_7", 1.1},
                    {"velocity_brightness", 5}};
  for (int i = 1; i <= 6; ++i)
    values["body_decay_active_" + std::to_string(i)] = 0;
  const double frequencies[]{128.9, 245,   304.7, 375,  421.9,  550.8,
                             621.1, 726.6, 878.9, 1380, 1699.2, 1980.5,
                             2900,  4200,  6000,  8800, 12000};
  const double levels[]{-12.12, -22.62, -16.47, -8.8, -16.09, -7.95,
                        -15.81, -13.31, -16.1,  -5.5, -5.5,   -7.3,
                        -5.5,   -4,     -4,     6,    0};
  for (int i = 0; i < 32; ++i) {
    const double f = i < 17 ? frequencies[i] : 15000;
    const auto suffix = std::to_string(i);
    values["resolved_frequency_" + suffix] = f;
    values["resolved_level_" + suffix] = i < 17 ? levels[i] : -72;
    values["resolved_turbulence_" + suffix] = i >= 17    ? 1
                                              : f < 1000 ? .12
                                              : f < 1500 ? .3
                                              : f < 1800 ? .4
                                              : f < 2500 ? .5
                                                         : 1.2;
  }
  return values;
}
SizeValues SmallSizeEndpoint() {
  SizeValues values{{"impact_tone_noise", .62},
                    {"impact_width", .65},
                    {"bloom_rate", 5},
                    {"bloom_energy_acceleration", .55},
                    {"bloom_energy_sensitivity", 1.1},
                    {"body_brightness", 1.5},
                    {"body_excitation_centre", 3000},
                    {"field_turbulence_slope", .12},
                    {"field_turbulence", .65 * std::pow(.25, .12)},
                    {"body_decay_seconds_0", 2.8},
                    {"body_decay_seconds_7", .35}};
  for (int i = 1; i <= 6; ++i)
    values["body_decay_active_" + std::to_string(i)] = 0;
  return values;
}
} // namespace drumfoundry::editing
