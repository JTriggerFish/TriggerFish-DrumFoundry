#include "analysis_panel.hpp"
#include <algorithm>
#include <cmath>
namespace drumfoundry::ui {
void AnalysisPanel::Menus() {
  fft_.onToggle() = [this](auto *, bool) {
    visage::PopupMenu menu;
    for (unsigned size = 256; size <= 32768; size *= 2)
      menu.addOption(int(size), "FFT " + std::to_string(size))
          .select(size == request_.transform.size);
    menu.onSelection() = [this](int size) {
      const double fraction =
          double(request_.transform.hop) / request_.transform.size;
      request_.transform.size = unsigned(size);
      request_.transform.hop = std::clamp(
          unsigned(std::lround(size * fraction)), 1u, unsigned(size));
      fft_.setText("FFT " + std::to_string(request_.transform.size));
      RefreshTransformLabels();
      Queue();
    };
    menu.show(&fft_);
  };
  window_.onToggle() = [this](auto *, bool) {
    visage::PopupMenu menu;
    menu.addOption(0, "Hann");
    menu.addOption(1, "Blackman–Harris");
    menu.addOption(2, "Rectangular");
    menu.onSelection() = [this](int i) {
      request_.transform.window = i == 0   ? "hann"
                                  : i == 1 ? "blackman-harris"
                                           : "rectangular";
      window_.setText(request_.transform.window);
      RefreshTransformLabels();
      Queue();
    };
    menu.show(&window_);
  };
  comparison_.onToggle() = [this](auto *, bool) {
    visage::PopupMenu menu;
    const char *names[]{"Mirror",     "Side by side", "Stacked",
                        "Difference", "Model",        "Reference"};
    for (int i = 0; i < 6; ++i)
      menu.addOption(i, names[i]).select(int(view_.comparison) == i);
    visage::PopupMenu range("Difference scale");
    for (int db : {6, 12, 24, 48})
      range.addOption(100 + db, "±" + std::to_string(db) + " dB")
          .select(view_.differenceDb == db);
    menu.addSubMenu(std::move(range));
    menu.onSelection() = [this](int i) {
      if (i >= 100) {
        view_.differenceDb = i - 100;
        view_.Refresh();
        return;
      }
      const char *names[]{"Mirror",     "Side by side", "Stacked",
                          "Difference", "Model",        "Reference"};
      view_.comparison = Comparison(i);
      comparison_.setText(names[i]);
      view_.Refresh();
    };
    menu.show(&comparison_);
  };
  channel_.onToggle() = [this](auto *, bool) {
    visage::PopupMenu menu;
    menu.addOption(0, "Mono average");
    menu.addOption(1, "Left channel");
    menu.addOption(2, "Right channel");
    menu.onSelection() = [this](int i) {
      request_.channel = analysis::Channel(i);
      channel_.setText(i == 0 ? "Mono average" : i == 1 ? "Left" : "Right");
      Queue();
    };
    menu.show(&channel_);
  };
}
} // namespace drumfoundry::ui
