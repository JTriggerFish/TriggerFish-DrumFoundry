#include "plugin.hpp"
#include <array>
#include <string>

namespace drumfoundry::clap_adapter {
bool Plugin::Save(const clap_ostream_t *stream) {
  if (!stream || !stream->write)
    return false;
  Json parameters = Json::object();
  const auto controls = DesiredControls();
  for (clap_id id = Preset; id < Reduction; ++id)
    parameters[std::to_string(id)] = controls[id - Preset];
  const auto data = Json{
      {"schema", "triggerfish.drumfoundry.clap-state/v1"},
      {"document", DesiredDocument()},
      {"parameters",
       parameters}}.dump();
  std::size_t offset = 0;
  while (offset < data.size()) {
    const auto count =
        stream->write(stream, data.data() + offset, data.size() - offset);
    if (count <= 0 || static_cast<uint64_t>(count) > data.size() - offset)
      return false;
    offset += static_cast<std::size_t>(count);
  }
  return true;
}
bool Plugin::Load(const clap_istream_t *stream) {
  if (!stream || !stream->read)
    return false;
  std::string data;
  std::array<char, 4096> chunk{};
  for (;;) {
    const auto count = stream->read(stream, chunk.data(), chunk.size());
    if (count < 0 || count > static_cast<int64_t>(chunk.size()))
      return false;
    if (!count)
      break;
    if (data.size() + count > 1024 * 1024)
      return false;
    data.append(chunk.data(), static_cast<std::size_t>(count));
  }
  auto state = ParseJson(data.c_str());
  if (state.at("schema") != "triggerfish.drumfoundry.clap-state/v1")
    return false;
  const auto &parameters = state.at("parameters");
  if (!parameters.is_object() || parameters.size() != Reduction - Preset)
    return false;
  std::array<double, Reduction - Preset> next{};
  for (clap_id id = Preset; id < Reduction; ++id) {
    const auto &value = parameters.at(std::to_string(id));
    if (!value.is_number() || !ValidValue(id, value.get<double>()))
      return false;
    next[id - Preset] = value.get<double>();
  }
  // Validate and expand off the audio thread. The active voice stays untouched
  // until the host stops processing and activates the replacement
  // configuration.
  Voice validated(static_cast<float>(sampleRate_), state.at("document"));
  auto document = validated.Document();
  document_ = std::move(document);
  documentPreset_ = static_cast<int>(next[0]);
  for (std::size_t i = 0; i < next.size(); ++i)
    values_[i].store(next[i]);
  if (active)
    RequestRestart();
  if (hostParams_ && hostParams_->rescan)
    hostParams_->rescan(host_, CLAP_PARAM_RESCAN_VALUES);
  return true;
}
} // namespace drumfoundry::clap_adapter
