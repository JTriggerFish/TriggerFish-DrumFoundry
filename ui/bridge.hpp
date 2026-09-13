#pragma once
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

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
  std::function<nlohmann::json()> document;
  std::function<void(const nlohmann::json &)> applyDocument;
  std::function<unsigned()> revision;
  std::function<void(const nlohmann::json &, const nlohmann::json &)>
      presentation;
  std::function<unsigned()> sampleRate;
  std::function<void(std::shared_ptr<const std::vector<float>>, unsigned,
                     double)>
      play;
};
} // namespace drumfoundry::ui
