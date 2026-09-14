#include "parameter_panel.hpp"
#include <algorithm>

namespace drumfoundry::ui {
namespace {
constexpr float Inset = 12, Gap = 12, HeaderHeight = 36;

visage::theme::ColorId SectionAccent(const std::string &section) {
  if (section == "Output")
    return colours::Muted;
  if (section == "Contact" || section == "Contact presentation" ||
      section == "Thump")
    return colours::Warning;
  if (section == "Bloom / energy travel" || section == "Strike / tension")
    return colours::Success;
  return colours::Accent;
}

class GroupHeading : public visage::Frame {
public:
  explicit GroupHeading(std::string text) : text_(std::move(text)) {}
  void draw(visage::Canvas &c) override {
    c.setColor(SectionAccent(text_));
    c.roundedRectangle(0, 11, 5, 12, 2);
    Label(c, ElideText(FrameFont(*this), text_, width() - 17), 17, 3,
          width() - 17, 26, colours::Heading);
  }

private:
  std::string text_;
};
} // namespace

void ParameterPanel::AddGroup(const std::string &section) {
  groups_.push_back({rows_.size()});
  auto heading = std::make_unique<GroupHeading>(section);
  addScrolledChild(heading.get());
  rows_.push_back({std::move(heading), int(HeaderHeight)});
}

void ParameterPanel::resized() {
  visage::ScrollableFrame::resized();
  float y = 0;
  const float rowWidth = std::max(1.f, width() - 14 - 2 * Inset);
  for (std::size_t i = 0; i < groups_.size(); ++i) {
    auto &group = groups_[i];
    group.top = y;
    const auto end =
        i + 1 < groups_.size() ? groups_[i + 1].firstRow : rows_.size();
    for (std::size_t j = group.firstRow; j < end; ++j) {
      auto &row = rows_[j];
      const auto *slider = dynamic_cast<Slider *>(row.frame);
      const int rowHeight =
          slider ? std::max(row.height,
                            int(slider->PreferredHeight(rowWidth)) + 4)
                 : row.height;
      row.frame->setBounds(Inset, y, rowWidth, rowHeight - 4);
      y += rowHeight;
      if (j == group.firstRow)
        y += 8;
    }
    y += 8;
    group.height = y - group.top;
    y += Gap;
  }
  setScrollableHeight(groups_.empty() ? 0 : y - Gap);
  redraw();
}

void ParameterPanel::draw(visage::Canvas &c) {
  const float w = std::max(1.f, width() - 14);
  for (const auto &group : groups_) {
    const float y = group.top - yPosition();
    if (y >= height() || y + group.height <= 0)
      continue;
    c.setColor(colours::Panel);
    c.roundedRectangle(0, y, w, group.height, 7);
    c.setColor(colours::Raised);
    c.roundedRectangle(1, y + 1, w - 2, HeaderHeight - 1, 6);
    c.fill(1, y + HeaderHeight - 7, w - 2, 7);
    c.setColor(colours::Border);
    c.roundedRectangleBorder(.5f, y + .5f, w - 1, group.height - 1, 7, 1);
  }
}
} // namespace drumfoundry::ui
