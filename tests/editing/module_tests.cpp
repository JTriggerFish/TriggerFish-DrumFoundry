#include "editing/document.hpp"
#include "patch/modules.hpp"
#include "runtime/modal_edit.hpp"
#include "runtime/voice.hpp"
#include <stdexcept>

namespace {
using namespace drumfoundry;
void Check(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
bool Rejected(Json patch) {
  try { Voice voice(48000, std::move(patch)); }
  catch (const std::exception &) { return true; }
  return false;
}
std::vector<float> Render(Json patch) {
  Voice voice(48000, std::move(patch));
  std::vector<float> result(24000);
  voice.Trigger({});
  voice.Process(result.data(), 6000);
  voice.Trigger({.4f});
  voice.Process(result.data() + 6000, result.size() - 6000);
  return result;
}
void Modules(const char *recipe) {
  editing::Document d;
  d.Load(DefaultPatch(recipe));
  const auto bypassed = Render(d.JsonValue());
  d.SetModule(RimContactType, false);
  Check(!HasRimContact(d.JsonValue()), "Module removal is real");
  Check(Render(d.JsonValue()) == bypassed, "Absent contact equals bypass exactly");
  for (const auto &p : d.Parameters())
    Check(p.key.rfind("hat_", 0) != 0, "Absent module has no editable controls");
  Voice absent(48000, d.JsonValue());
  Check(!absent.StageParameter(detail::RimParameterFirst(ParseRecipe(recipe)), 1),
        "Automation must not activate an absent module");
  const auto without = d.JsonValue();
  d.SetModule(RimContactType, true);
  Check(d.Description("hat_openness").owner == RimContactId, "One parameter owner");
  d.Set("hat_openness", 0);
  d.Set("hat_contact_loss", .7);
  Check(Render(d.JsonValue()) != bypassed, "Attached contact changes the body");
  auto bad = d.JsonValue();
  bad["attachments"][0]["port"] = "audio";
  Check(Rejected(bad), "Audio ports cannot masquerade as body energy");
  bad = d.JsonValue();
  bad["attachments"][0]["body"] = "output";
  Check(Rejected(bad), "Contact only attaches to a compatible resonator");
  bad = without;
  bad["attachments"] = d.JsonValue().at("attachments");
  Check(Rejected(bad), "Dangling attachment rejected");
  d.SetModule(RimContactType, false);
  Check(d.JsonValue() == without, "Add/remove leaves other modules unchanged");
}
void Upgrade() {
  auto modern = DefaultPatch("metal.cymbal.v1");
  modern["nodes"].back()["parameters"]["hat_contact_enabled"] = 1;
  auto legacy = modern;
  const auto parameters = legacy["nodes"].back().at("parameters");
  legacy["nodes"].erase(legacy["nodes"].end() - 1);
  legacy.erase("attachments");
  for (auto &node : legacy["nodes"])
    if (node.at("id") == "body") node["parameters"].update(parameters);
  Check(Render(legacy) == Render(modern), "Legacy migration preserves retrigger audio");
  Voice migrated(48000, legacy);
  Check(migrated.Document() == modern, "Legacy parameters move without numerical edits");
  // Worker-prepared edits use the same upgrader, not a second ownership path.
  Check(bool(PrepareModalEdit(48000, legacy)), "Legacy native prepared editing");
  legacy["nodes"].push_back(RimContactNode());
  Check(Rejected(legacy), "Conflicting old/new parameter owners rejected");
}
} // namespace
void ModuleTests() {
  for (auto recipe : {"metal.cymbal.v1", "drum.kick.v1", "drum.membrane.v1", "drum.snare.v1"})
    Modules(recipe);
  Upgrade();
}
