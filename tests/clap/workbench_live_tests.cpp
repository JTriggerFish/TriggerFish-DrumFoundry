#include "adapters/clap/plugin.hpp"
#include "ui/eq_plot.hpp"
#include "ui/workbench.hpp"
#include <cmath>
#include <stdexcept>

namespace {
void Check(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
template <class T> T *Find(visage::Frame &f) {
  if (auto *p = dynamic_cast<T *>(&f))
    return p;
  for (auto *c : f.children())
    if (auto *p = Find<T>(*c))
      return p;
  return nullptr;
}
} // namespace
void WorkbenchLiveTests() {
  using namespace drumfoundry;
  clap_host_t host{CLAP_VERSION, nullptr, "Test", "TriggerFish", "", "1"};
  auto plugin = std::make_unique<clap_adapter::Plugin>(&host);
  Check(plugin->Init(), "UI live init");
  auto document = plugin->EditableDocument();
  document["controls"]["analysis"]["view"] = {{"renderSeconds", .25}};
  plugin->EditDocument(document);
  ui::Bridge b;
  b.history = plugin->editHistory;
  b.document = [&] { return plugin->EditableDocument(); };
  b.value = [&](unsigned id) { return plugin->Value(id); };
  b.revision = [&] { return plugin->DocumentRevision(); };
  b.applyDocument = [&](const auto &d) { plugin->EditDocument(d); };
  b.liveDocument = [&](const auto &d) { return plugin->EditLiveDocument(d); };
  b.finishLive = [&] { plugin->EndDesignGesture(); };
  b.automationRevision = [&] { return plugin->AutomationRevision(); };
  auto editor = std::make_unique<ui::Workbench>(b);
  editor->setBounds(0, 0, 1200, 800);
  auto *eq = Find<ui::EqPlot>(*editor);
  Check(eq, "Missing live EQ");
  editing::Document before;
  before.Load(plugin->EditableDocument());
  visage::MouseEvent e;
  e.button_id = visage::kMouseButtonLeft;
  e.repeat_click_count = 1;
  e.position = {30 + (eq->width() - 38) *
                         float(std::log(before.Value("output_low_cut") / 5) /
                               std::log(4400.)),
                32 + (eq->height() - 137) * .4f};
  Check(before.Value("output_low_cut") == 5, "Expected boundary EQ default");
  const auto origin = e.position;
  eq->mouseDown(e);
  e.position.x += 10;
  eq->mouseDrag(e);
  e.position.x =
      0; // Clamp exactly to the original 5 Hz, not a float roundtrip.
  eq->mouseDrag(e);
  eq->mouseUp(e);
  Check(plugin->EditableDocument() == before.JsonValue() &&
            !b.history->CanUndo(),
        "Returning a live drag to its start left a stale value/history");
  e.position = origin;
  eq->mouseDown(e);
  for (int i = 0; i < 5; ++i) {
    e.position.x += 3;
    eq->mouseDrag(e);
    Check(plugin->EditableDocument() != before.JsonValue(),
          "EQ did not publish while the mouse was held");
    Check(!b.history->CanUndo(), "Drag generated an undo step before release");
  }
  eq->mouseUp(e);
  const auto after = plugin->EditableDocument();
  Check(b.history->CanUndo(), "EQ gesture missing from history");
  editor->Undo();
  Check(plugin->EditableDocument() == before.JsonValue() &&
            !b.history->CanUndo(),
        "Live EQ gesture did not undo in one step");
  editor->Redo();
  Check(plugin->EditableDocument() == after,
        "Live EQ redo lost final position");
  // DAW automation during a UI gesture must survive both commit and undo.
  plugin->DrainEditor(nullptr, false);
  clap_id thump{};
  for (const auto &p : clap_adapter::DesignParameters())
    if (p.recipe == detail::Recipe::Kick &&
        p.descriptor->key == "thump_pitch_hz")
      thump = p.id;
  Check(thump, "Missing thump automation ID");
  eq->mouseDown(e);
  e.position.x += 5;
  eq->mouseDrag(e);
  plugin->SetParameter(thump, 77);
  e.position.x += 5;
  eq->mouseDrag(e);
  eq->mouseUp(e);
  editing::Document automated;
  automated.Load(plugin->EditableDocument());
  Check(automated.Value("thump_pitch_hz") == 77,
        "UI edit rewound DAW automation");
  eq->mouseDown(e);
  e.position.x += 4;
  eq->mouseDrag(e);
  eq->mouseUp(e);
  automated.Load(plugin->EditableDocument());
  Check(automated.Value("thump_pitch_hz") == 77,
        "The next gesture mistook stale controls for edits");
  editor->Undo();
  editor->Undo();
  automated.Load(plugin->EditableDocument());
  Check(automated.Value("thump_pitch_hz") == 77,
        "Undo rewound unrelated DAW automation");

  // ModalPanel must forward continuous plot edits, not only mouse-up commits.
  editor.reset();
  plugin->SelectFactory(5);
  editor = std::make_unique<ui::Workbench>(b);
  editor->setBounds(0, 0, 1200, 900);
  auto *modal = Find<ui::ModalPlot>(*editor);
  Check(modal, "Missing live modal editor");
  editing::Document gong;
  gong.Load(plugin->EditableDocument());
  const auto modes = editing::Modes(gong);
  const auto m = modes.at(0);
  e.position = {42 + float(std::log(m.frequency / 20) / std::log(1000.)) *
                         (modal->width() - 60),
                14 + float((6 - m.level) / 78) * (modal->height() - 44)};
  const auto modalOrigin = e.position;
  b.history->Clear();
  for (auto tool : {ui::ModalPlot::Tool::Edit, ui::ModalPlot::Tool::Shape,
                    ui::ModalPlot::Tool::Paint}) {
    modal->tool = tool;
    e.position = modalOrigin;
    modal->mouseDown(e);
    e.position.x += 10;
    e.position.y += 6;
    modal->mouseDrag(e);
    Check(plugin->EditableDocument() != gong.JsonValue(),
          "Cancellation test must first publish a live edit");
    plugin->DrainEditor(nullptr, false);
    modal->keyPress({visage::KeyCode::Escape, 0, true});
    modal->mouseUp(e);
    Check(plugin->EditableDocument() == gong.JsonValue(),
          "Escape restored picture but not the live host document");
    Check(!b.history->CanUndo(), "Cancelled modal edit left an undo entry");
    editor->Undo(); // Must not finish a stale gesture and resurrect the edit.
    Check(plugin->EditableDocument() == gong.JsonValue(),
          "Cancelled gesture remained open");
  }
  modal->tool = ui::ModalPlot::Tool::Edit;
  e.position = modalOrigin;
  modal->mouseDown(e);
  e.position.x += 10;
  modal->mouseDrag(e);
  Check(plugin->EditableDocument() != gong.JsonValue(),
        "Modal drag did not publish before release");
  modal->mouseUp(e);
  const auto moved = plugin->EditableDocument();
  editor->Undo();
  Check(plugin->EditableDocument() == gong.JsonValue(),
        "Modal drag undo failed");
  editor->Redo();
  Check(plugin->EditableDocument() == moved, "Modal drag redo failed");
}
