#pragma once
#include "controls.hpp"
#include "workbench/decay_hold/worker.hpp"
#include <chrono>
namespace drumfoundry::ui {
// Borrowed by the bloom column; survives parameter-panel rebuilds.
class DecayHoldPanel : public visage::Frame {
public:
  DecayHoldPanel();
  void Edited(const editing::Json &before, const editing::Json &after,
              unsigned rate);
  void Poll(const editing::Json &current);
  void Cancel();
  bool NeedsPoll() const { return pending_ || waiting_; }
  void resized() override;
  void draw(visage::Canvas &) override;
  std::function<void(const decay_hold::Result &)> apply;
  std::function<void(const std::string &)> error;

private:
  static editing::Json Sound(const editing::Json &);
  HelpButton enabled_{"Hold decay ON"}, cancel_{"Cancel"};
  decay_hold::Worker worker_;
  editing::Json baseline_, edited_, expected_;
  std::chrono::steady_clock::time_point started_;
  unsigned rate_{48000};
  bool enabledValue_{true}, pending_{}, waiting_{};
  std::string status_;
};
} // namespace drumfoundry::ui
