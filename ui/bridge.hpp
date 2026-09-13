#pragma once
#include "adapters/shared/audio_tap.hpp"
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
  std::function<void(unsigned)> selectFactory;
  std::function<void(float, float)> strike;
  std::function<double()> velocity;
  std::function<void(double)> setVelocity;
  std::function<void()> stop;
  std::function<void()> service;
  std::function<void()> settings;
  std::function<std::string()> status;
  std::function<nlohmann::json()> document;
  std::function<void(const nlohmann::json &)> applyDocument;
  std::function<unsigned()> revision;
  std::function<void(const nlohmann::json &)> layout;
  std::function<void(const nlohmann::json &, const nlohmann::json &)>
      presentation;
  std::function<unsigned()> sampleRate;
  std::function<host::TapRead(float *, unsigned)> readOutput;
  std::function<host::TapRead(float *, unsigned)> readVoice;
  std::function<void(std::shared_ptr<const std::vector<float>>, unsigned,
                     double)>
      play;
};
} // namespace drumfoundry::ui
