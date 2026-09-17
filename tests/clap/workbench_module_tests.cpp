#include "adapters/clap/plugin.hpp"
#include "patch/modules.hpp"
#include "ui/workbench.hpp"
#include <chrono>
#include <stdexcept>
#include <thread>

namespace {
using namespace drumfoundry;
void Check(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
template <class T> T *Find(visage::Frame &frame) {
  if (auto *match = dynamic_cast<T *>(&frame)) return match;
  for (auto *child : frame.children())
    if (auto *match = Find<T>(*child)) return match;
  return nullptr;
}
void PumpUi() {
  // Exercise the real deferred rebuild path without exposing private methods.
  std::this_thread::sleep_for(std::chrono::milliseconds(40));
  visage::EventManager::instance().checkEventTimers();
}
} // namespace

void WorkbenchModuleTests() {
  clap_host_t host{CLAP_VERSION, nullptr, "Modules", "TriggerFish", "", "1"};
  auto plugin = std::make_unique<clap_adapter::Plugin>(&host);
  Check(plugin->Init(), "Module workbench init");
  plugin->SelectFactory(0);
  auto document = plugin->EditableDocument();
  document["controls"]["analysis"]["view"] = {{"renderSeconds", .25}};
  plugin->EditDocument(document);
  ui::Bridge b;
  b.history = plugin->editHistory;
  b.document = [&] { return plugin->EditableDocument(); };
  b.value = [&](unsigned id) { return plugin->Value(id); };
  b.revision = [&] { return plugin->DocumentRevision(); };
  b.applyDocument = [&](const auto &d) { plugin->EditDocument(d); };
  b.restoreDocument = [&](const auto &d, unsigned preset) { plugin->EditDocument(d, int(preset)); };
  auto editor = std::make_unique<ui::Workbench>(b);
  editor->setBounds(0, 0, 1200, 800);
  editor->OpenRouting();
  auto *routing = Find<ui::RoutingPanel>(*editor);
  Check(routing, "Missing topology panel");
  const auto original = plugin->EditableDocument();
  routing->SetModule(RimContactType, true);
  PumpUi(); // Rebuild owners/rows; must not access removed controls.
  const auto added = plugin->EditableDocument();
  Check(HasRimContact(added.at("instrument")), "UI module add was lost");
  Check(b.history->CanUndo(), "Module add was not recorded");
  editor->Undo();
  PumpUi();
  Check(plugin->EditableDocument() == original, "Module undo changed other settings");
  editor->Redo();
  PumpUi();
  Check(plugin->EditableDocument() == added, "Module redo failed");
  routing->SetModule(RimContactType, false);
  PumpUi();
  Check(plugin->EditableDocument() == original, "UI module removal failed");
  editor->Undo();
  PumpUi();
  Check(plugin->EditableDocument() == added, "Removal undo lost contact settings");
}
