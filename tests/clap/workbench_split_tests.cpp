#include "ui/analysis_validation.hpp"
#include "ui/workbench.hpp"
#include <cmath>
#include <stdexcept>

namespace {
using namespace drumfoundry::ui;
void Check(bool ok) {
  if (!ok)
    throw std::runtime_error("Workbench divider regression");
}
SplitBar *Find(visage::Frame &frame, bool vertical) {
  if (auto *split = dynamic_cast<SplitBar *>(&frame))
    if (split->vertical == vertical)
      return split;
  for (auto *child : frame.children())
    if (auto *split = Find(*child, vertical))
      return split;
  return nullptr;
}
void Drag(Workbench &editor, SplitBar &split, float delta) {
  const auto before = split.positionInWindow();
  visage::MouseEvent event;
  event.button_id = visage::kMouseButtonLeft;
  event.position = {split.width() / 2, split.height() / 2};
  event.window_position = before + event.position;
  Check(editor.frameAtPoint(event.window_position -
                            editor.positionInWindow()) == &split);
  split.processMouseDown(event);
  if (split.vertical)
    event.window_position.x += delta;
  else
    event.window_position.y += delta;
  split.processMouseDrag(event);
  const auto after = split.positionInWindow();
  Check(std::abs((split.vertical ? after.x - before.x : after.y - before.y) -
                 delta) < .01);
  if (split.vertical)
    event.window_position.x -= delta;
  else
    event.window_position.y -= delta;
  split.processMouseDrag(event);
  split.processMouseUp(event);
  Check(std::abs(split.positionInWindow().x - before.x) < .01);
  Check(std::abs(split.positionInWindow().y - before.y) < .01);
}
} // namespace
void WorkbenchSplitTests(Workbench &editor, AnalysisPanel &analysis) {
  auto *vertical = Find(editor, true), *horizontal = Find(editor, false);
  Check(vertical && horizontal);
  for (int size : {0, 1, 2}) {
    editor.setBounds(0, 0, 1600, 1200);
    editor.SetTextSize(size);
    editor.SetControlWidth(700);
    editor.SetVisualPanels(true, true);
    analysis.analysisShare =
        .1; // Start at the actual minimum, not a stale ratio.
    editor.resized();
    Drag(editor, *vertical, 40);
    Drag(editor, *horizontal, 40);
    Check(!ReadSavedView(analysis.Settings()).is_null());
    horizontal->started();
    horizontal->dragged(1000000);
    Check(analysis.analysisShare == .9);
    Check(!ReadSavedView(analysis.Settings()).is_null());
    horizontal->dragged(-1000000);
    Check(!ReadSavedView(analysis.Settings()).is_null());
  }
  editor.SetVisualPanels(false, true);
  Check(!horizontal->isVisible() && vertical->isVisible());
  editor.SetVisualPanels(true, true);
  editor.SetTextSize(0);
}
