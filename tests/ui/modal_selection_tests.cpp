#include "ui/modal_plot.hpp"
#include <cmath>
#include <stdexcept>

namespace {
using namespace drumfoundry;
void Check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
bool Near(double a, double b) {
  // Screen coordinates are floats; log-frequency round trips need relative
  // tolerance.
  return std::abs(a - b) < 1e-4 + 5e-7 * std::abs(b);
}
visage::Point Point(double f, double level) {
  return {42 + float(std::log(f / 20) / std::log(1000.)) * 740,
          14 + float((6 - level) / 78) * 256};
}
struct Editor {
  editing::Document d;
  ui::ModalPlot plot;
  unsigned commits{}, errors{};
  explicit Editor(editing::Document source) : d(std::move(source)) {
    editing::ReplaceModes(
        d, {{200, -12, .5, 1}, {400, -18, 1, 1}, {2000, -6, .7, 1}});
    plot.setBounds(0, 0, 800, 300);
    plot.Load(d);
    plot.committed = [this] { ++commits; };
    plot.error = [this](const auto &) { ++errors; };
  }
  visage::MouseEvent Event(visage::Point p, int modifiers = 0, int clicks = 1) {
    visage::MouseEvent e;
    e.button_id = visage::kMouseButtonLeft;
    e.position = p;
    e.modifiers = modifiers;
    e.repeat_click_count = clicks;
    return e;
  }
  void Drag(visage::Point from, visage::Point to, int modifiers = 0) {
    auto e = Event(from, modifiers);
    plot.processMouseDown(e);
    e.position = to;
    plot.processMouseDrag(e);
    plot.processMouseUp(e);
  }
  void Box() { Drag(Point(150, -4), Point(500, -24)); }
  void Click(visage::Point p, int clicks = 1, int modifiers = 0) {
    auto e = Event(p, modifiers, clicks);
    plot.processMouseDown(e);
    plot.processMouseUp(e);
  }
};
void GroupMovement(editing::Document d) {
  Editor e(d);
  const auto before = e.d.JsonValue();
  e.Box();
  Check(e.plot.SelectionCount() == 2 && e.plot.IsSelected(0) &&
            e.plot.IsSelected(1),
        "Rectangle must select both handles");
  Check(e.d.JsonValue() == before && e.commits == 0,
        "Selection alone must not edit or re-render the sound");
  e.Drag(Point(200, -12), Point(300, -9));
  auto modes = editing::Modes(e.d);
  Check(Near(modes[0].frequency, 300) && Near(modes[1].frequency, 600) &&
            Near(modes[0].level, -9) && Near(modes[1].level, -15),
        "Group drag must preserve ratios and relative dB");
  Check(modes[2].frequency == 2000 && modes[2].level == -6 && e.commits == 1,
        "Unselected mode must not move; one commit per drag");
  // Clamp as one group, then return to the original mouse position. No drift.
  auto event = e.Event(Point(300, -9));
  e.plot.processMouseDown(event);
  event.position = {4000, -4000};
  e.plot.processMouseDrag(event);
  modes = editing::Modes(e.d);
  Check(Near(modes[1].frequency, 20000) && Near(modes[0].frequency, 10000) &&
            Near(modes[0].level, 6) && Near(modes[1].level, 0),
        "Clamping must not squeeze the group");
  event.position = Point(300, -9);
  e.plot.processMouseDrag(event);
  e.plot.processMouseUp(event);
  modes = editing::Modes(e.d);
  Check(Near(modes[0].frequency, 300) && Near(modes[0].level, -9),
        "Dragging back from a limit must restore the original position");
  e.Drag(Point(300, -9), {-1000, 1000});
  modes = editing::Modes(e.d);
  Check(Near(modes[0].frequency, 20) && Near(modes[1].frequency, 40) &&
            Near(modes[1].level, -71.9) && modes[0].level > -72,
        "Lower clamping must preserve the group and not disable modes");
}
void FilledSelection(editing::Document d) {
  Editor e(d);
  e.Box();
  // Empty space between selected stems is a persistent, highlighted grab area.
  e.Drag(Point(300, -40), Point(450, -37));
  auto modes = editing::Modes(e.d);
  Check(
      Near(modes[0].frequency, 300) && Near(modes[1].frequency, 600) &&
          Near(modes[0].level, -9) && e.plot.SelectionCount() == 2,
      "Dragging inside the highlighted group must move it without reselection");
  e.plot.Refresh();
  e.Drag(Point(450, -40), Point(300, -43));
  modes = editing::Modes(e.d);
  Check(Near(modes[0].frequency, 200) && Near(modes[1].frequency, 400),
        "Selection grab area must follow movement and survive refresh");
  // A single Gaussian wing is also yellow, not just its centre line.
  e.plot.keyPress({visage::KeyCode::Escape, 0, true});
  e.d.Set("field_turbulence", 1);
  e.d.Set("field_packet_spread", 2);
  e.plot.Refresh();
  e.Click(Point(400, -18));
  e.Drag(Point(440, -60), Point(550, -60));
  modes = editing::Modes(e.d);
  Check(Near(modes[1].frequency, 500) && e.plot.SelectionCount() == 1,
        "The selected Gaussian wing must be draggable");
}
void SelectionAndWidth(editing::Document d) {
  Editor e(d);
  // Reverse-direction rectangles select identically.
  e.Drag(Point(500, -24), Point(150, -4));
  Check(e.plot.SelectionCount() == 2, "Reverse marquee selection failed");
  auto wheel = e.Event(Point(400, -18), visage::kModifierRegCtrl);
  wheel.precise_wheel_delta_y = 5;
  Check(e.plot.mouseWheel(wheel), "Ctrl-scroll should be handled");
  auto modes = editing::Modes(e.d);
  Check(Near(modes[0].turbulence, .6) && Near(modes[1].turbulence, 1.1) &&
            Near(modes[2].turbulence, .7),
        "Ctrl-scroll must widen the selection");
  wheel.precise_wheel_delta_y = -5;
  e.plot.mouseWheel(wheel);
  modes = editing::Modes(e.d);
  Check(Near(modes[0].turbulence, .5) && Near(modes[1].turbulence, 1),
        "Opposite wheel movement should restore widths");
  wheel.modifiers = 0;
  wheel.precise_wheel_delta_y = 5;
  e.plot.mouseWheel(wheel);
  modes = editing::Modes(e.d);
  Check(Near(modes[0].turbulence, .5) && Near(modes[1].turbulence, 1.1),
        "Plain scroll should affect only the active handle");
  e.Click(Point(2000, -6), 1, visage::kModifierShift);
  Check(e.plot.SelectionCount() == 3, "Shift-click must add a handle");
  e.plot.keyPress({visage::KeyCode::Delete, 0, true});
  Check(e.plot.SelectionCount() == 0, "Delete must clear the selection");
  for (auto m : editing::Modes(e.d))
    Check(m.level == -72, "Delete must remove every selected mode");
}
void Insertions(editing::Document d) {
  Editor e(d);
  e.Click(Point(19000, -24), 2);
  Check(Near(editing::Modes(e.d)[3].frequency, 19000),
        "Upper octave supports double-click insertion");
  e.Click(Point(19000, -24), 2);
  e.commits = 0;
  // A real double-click includes the first click and both releases.
  e.Click(Point(900, -24));
  e.Click(Point(900, -24), 2);
  Check(editing::Modes(e.d)[3].level > -72 && e.commits == 1,
        "Double-click in empty space must insert exactly one mode");
  e.Click(Point(900, -24));
  e.Click(Point(900, -24), 2);
  Check(editing::Modes(e.d)[3].level == -72 && e.commits == 2,
        "Double-click on a circular handle must remove it");
  e.Click(Point(200, -40));
  e.Click(Point(200, -40), 2);
  Check(editing::Modes(e.d)[0].level == -12 &&
            Near(editing::Modes(e.d)[3].level, -40),
        "Double-click below a stem must insert, not delete the tall mode");
  auto full = editing::Modes(e.d);
  for (auto &m : full)
    m = {400, -6, 1, 1};
  editing::ReplaceModes(e.d, full);
  e.plot.Refresh();
  const auto before = e.d.JsonValue();
  e.Click(Point(900, -24));
  e.Click(Point(900, -24), 2);
  Check(e.errors == 1 && e.d.JsonValue() == before,
        "Full bank must report capacity without silently deleting modes");
  e.plot.Load(d);
  Check(e.plot.SelectionCount() == 0, "Loading a preset must clear selection");
}
void FineAndSnappedMovement(editing::Document d) {
  Editor e(d);
  e.Box();
  e.Drag(Point(200, -12), Point(400, -2), visage::kModifierShift);
  auto modes = editing::Modes(e.d);
  Check(Near(modes[0].frequency, 200 * std::pow(2., .1)) &&
            Near(modes[0].level, -11),
        "Shift must provide fine group movement");
  editing::ReplaceModes(
      e.d, {{210, -12, .5, 1}, {400, -18, 1, 1}, {2000, -6, .7, 1}});
  e.plot.Refresh();
  e.plot.guide = e.plot.snap = true;
  e.plot.base = 100;
  e.Drag(Point(210, -12), Point(295, -12));
  modes = editing::Modes(e.d);
  Check(Near(modes[0].frequency, 300) &&
            Near(modes[1].frequency / modes[0].frequency, 400. / 210),
        "Snapping must use the active handle without retuning group intervals");
  e.Drag(Point(1700, 0), Point(2200, -10), visage::kModifierShift);
  Check(e.plot.SelectionCount() == 3, "Shift-marquee must add to selection");
  const auto before = e.d.JsonValue();
  e.plot.keyPress({visage::KeyCode::Escape, 0, true});
  Check(e.plot.SelectionCount() == 0 && e.d.JsonValue() == before,
        "Escape must clear selection without changing sound");
}
} // namespace
void ModalSelectionTests(drumfoundry::editing::Document d) {
  for (auto tool : {drumfoundry::ui::ModalPlot::Tool::Edit,
                    drumfoundry::ui::ModalPlot::Tool::Paint,
                    drumfoundry::ui::ModalPlot::Tool::Shape}) {
    Editor e(d);
    e.plot.tool = tool;
    const auto before = e.d.JsonValue();
    auto event = e.Event(Point(400, -18));
    e.plot.processMouseDown(event);
    event.position = Point(500, -8);
    e.plot.processMouseDrag(event);
    Check(e.d.JsonValue() != before, "Cancel test must first change modes");
    e.plot.keyPress({visage::KeyCode::Escape, 0, true});
    e.plot.processMouseUp(event);
    Check(e.d.JsonValue() == before && e.commits == 0,
          "Escape must restore the gesture baseline without a hidden commit");
    e.Drag(Point(400, -18), Point(450, -12));
    Check(e.commits == 1,
          "A new gesture after cancellation must commit normally");
  }
  GroupMovement(d);
  FilledSelection(d);
  SelectionAndWidth(d);
  Insertions(d);
  FineAndSnappedMovement(d);
}
