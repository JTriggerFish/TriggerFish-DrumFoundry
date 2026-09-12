#include "native.h"
#include "runtime/voice.hpp"
#include <cmath>
#include <exception>
#include <string>

struct df_voice {
  drumfoundry::Voice voice;
};
namespace {
thread_local std::string error, text;
template <class Action> int Checked(Action action) noexcept {
  try {
    action();
    error.clear();
    return 1;
  } catch (const std::exception &e) {
    error = e.what();
  } catch (...) {
    error = "Unknown native engine error";
  }
  return 0;
}
void Require(const void *pointer) {
  if (!pointer)
    throw std::invalid_argument("Null native argument");
}
} // namespace
extern "C" {
const char *df_last_error() { return error.c_str(); }
df_voice *df_create(float sampleRate, const char *document) {
  df_voice *result = nullptr;
  Checked([&] {
    result = new df_voice{
        drumfoundry::Voice(sampleRate, drumfoundry::ParseJson(document))};
  });
  return result;
}
void df_destroy(df_voice *p) { delete p; }
int df_configure(df_voice *p, const char *document) {
  return Checked([&] {
    Require(p);
    p->voice.Configure(drumfoundry::ParseJson(document));
  });
}
const char *df_document(df_voice *p) {
  return Checked([&] {
    Require(p);
    text = p->voice.Document().dump();
  })
             ? text.c_str()
             : nullptr;
}
const char *df_descriptors(df_voice *p) {
  return Checked([&] {
    Require(p);
    text = p->voice.Descriptors().dump();
  })
             ? text.c_str()
             : nullptr;
}
const char *df_default_patch(const char *recipe) {
  return Checked([&] {
    Require(recipe);
    text = drumfoundry::DefaultPatch(recipe).dump();
  })
             ? text.c_str()
             : nullptr;
}
int df_reset(df_voice *p) {
  if (!p)
    return 0;
  p->voice.Reset();
  return 1;
}
int df_default_strike(df_voice *p, df_strike *destination) {
  if (!p || !destination)
    return 0;
  const auto &e = p->voice.Event();
  *destination = {e.strength,      e.location,   e.hardness, e.implement,
                  e.contactSpread, e.constraint, e.seed};
  return 1;
}
int df_trigger(df_voice *p, const df_strike *event) {
  return Checked([&] {
    Require(p);
    Require(event);
    drumfoundry::Strike e{
        event->strength,  event->location,       event->hardness,
        event->implement, event->contact_spread, event->constraint,
        event->seed};
    drumfoundry::ValidateStrike(e);
    p->voice.Trigger(e);
  });
}
int df_process(df_voice *p, float *output, uint32_t frames) {
  if (!p || (!output && frames))
    return 0;
  p->voice.Process(output, frames);
  return 1;
}
int df_set_mute(df_voice *p, float amount) {
  if (!p || !std::isfinite(amount) || amount < 0 || amount > 1)
    return 0;
  p->voice.SetMute(amount);
  return 1;
}
}
