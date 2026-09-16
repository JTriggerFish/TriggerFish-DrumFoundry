#include "adapters/clap/plugin.hpp"
#include "ui/workbench.hpp"
#include <stdexcept>

namespace {
void Check(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
} // namespace
void WorkbenchHistoryPresets() {
  using namespace drumfoundry;
  clap_host_t host{CLAP_VERSION, nullptr, "Test", "TriggerFish", "", "1"};
  auto plugin = std::make_unique<clap_adapter::Plugin>(&host);
  Check(plugin->Init(), "Preset history initialization failed");
  ui::Bridge b;
  b.history = plugin->editHistory;
  b.value = [&](unsigned id) { return plugin->EditorValue(id); };
  b.document = [&] { return plugin->EditableDocument(); };
  b.applyDocument = [&](const auto &d) { plugin->EditDocument(d); };
  b.restoreDocument = [&](const auto &d, unsigned preset) {
    plugin->EditDocument(d, int(preset));
  };
  b.selectFactory = [&](unsigned preset) { plugin->SelectFactory(preset); };
  // Include same-recipe metal transitions and transitions from drum recipes.
  for (unsigned previous : {5u, 4u, 3u, 2u, 1u, 0u}) {
    plugin->SelectFactory(previous);
    auto edited = plugin->EditableDocument();
    edited["name"] = "Custom edited instrument";
    edited["controls"]["event"]["hardness"] = .21;
    edited["controls"]["analysis"]["view"] = {{"renderSeconds", .25}};
    plugin->EditDocument(edited);
    edited = plugin->EditableDocument();
    b.history->Clear();
    auto editor = std::make_unique<ui::Workbench>(b);
    const unsigned next = previous == 5 ? 3 : 5;
    editor->LoadFactoryPreset(next);
    const auto loaded = plugin->EditableDocument();
    // Undo preserves the current analysis viewport rather than its old
    // snapshot.
    edited["controls"]["analysis"] = loaded.at("controls").at("analysis");
    editor->Undo();
    if (plugin->Value(clap_adapter::Preset) != previous ||
        plugin->EditableDocument() != edited)
      throw std::runtime_error(
          "Undo selector/document mismatch: " + std::to_string(previous) +
          " vs " + std::to_string(plugin->Value(clap_adapter::Preset)) + " " +
          Json::diff(edited, plugin->EditableDocument()).dump());
    editor->Redo();
    Check(plugin->Value(clap_adapter::Preset) == next &&
              plugin->EditableDocument() == loaded,
          "Redo must restore the selected factory and document");
  }
  plugin->QueueEdit(clap_adapter::Hardness, .1);
  plugin->SelectFactory(0);
  const auto hardness = plugin->Value(clap_adapter::Hardness);
  plugin->DrainEditor(nullptr, false);
  Check(plugin->EditorValue(clap_adapter::Hardness) == hardness,
        "An obsolete queued edit must not overwrite a newly loaded preset");
  const auto before = plugin->EditableDocument();
  bool rejected = false;
  try {
    plugin->EditDocument(before, 5);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  Check(rejected && plugin->EditableDocument() == before &&
            plugin->Value(clap_adapter::Preset) == 0,
        "Incompatible selector restore must reject without changing state");
}
