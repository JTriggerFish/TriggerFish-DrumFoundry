#pragma once
#include "patch/document.hpp"
#include <memory>

namespace drumfoundry {
// One owned voice, used identically by offline renderers and future host
// adapters. Configure is transactional and non-realtime. Trigger/Process do not
// allocate. A caller must serialize access to one voice; separate voices are
// independent.
class Voice {
public:
  Voice(float sampleRate, Json document);
  void Configure(Json document);
  void Reset() noexcept;
  void Trigger(const Strike &) noexcept;
  void SetMute(float amount) noexcept;
  void Process(float *output, std::size_t frames) noexcept;
  const Json &Document() const noexcept { return document_; }
  const Strike &Event() const noexcept { return event_; }
  Json Descriptors() const { return DescribeParameters(*session_); }

private:
  float sampleRate_;
  Json document_;
  Strike event_;
  std::unique_ptr<detail::Session> session_;
};
} // namespace drumfoundry
