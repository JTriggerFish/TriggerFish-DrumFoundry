#pragma once
#include <functional>
#include <string>

namespace drumfoundry::ui {
// Main-thread UI boundary. Hosts enqueue performance edits; structural edits
// use their preparation lifecycle. No widget reaches into a live DSP voice.
struct Bridge {
  std::function<double(unsigned)> value;
  std::function<void(unsigned, double)> change;
  std::function<void(float, float)> strike;
  std::function<void()> stop;
  std::function<void()> service;
  std::function<void()> settings;
  std::function<std::string()> status;
};
} // namespace drumfoundry::ui
