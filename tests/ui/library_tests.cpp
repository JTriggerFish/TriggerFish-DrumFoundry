#include "ui/analysis_panel.hpp"
#include "workbench/analysis/library.hpp"
#include <chrono>
#include <fstream>
#include <random>

namespace {
void Check(bool ok) {
  if (!ok)
    throw std::runtime_error("Reference library regression");
}
template <class F> void Reject(F action) {
  bool rejected = false;
  try {
    action();
  } catch (const std::exception &) {
    rejected = true;
  }
  Check(rejected);
}
void Little(std::ostream &out, unsigned value, unsigned bytes) {
  for (unsigned i = 0; i < bytes; ++i)
    out.put(char(value >> (8 * i)));
}
void Wave(const std::filesystem::path &path) {
  std::ofstream out(path, std::ios::binary);
  out.write("RIFF", 4);
  Little(out, 36 + 8820, 4);
  out.write("WAVEfmt ", 8);
  Little(out, 16, 4);
  Little(out, 1, 2);
  Little(out, 1, 2);
  Little(out, 44100, 4);
  Little(out, 88200, 4);
  Little(out, 2, 2);
  Little(out, 16, 2);
  out.write("data", 4);
  Little(out, 8820, 4);
  for (unsigned i = 0; i < 4410; ++i)
    Little(out, i % 64 < 32 ? 5000 : unsigned(-5000), 2);
}
void Finish(drumfoundry::ui::AnalysisPanel &panel) {
  const auto until =
      std::chrono::steady_clock::now() + std::chrono::seconds(10);
  do {
    panel.Poll();
    if (panel.Ready())
      return;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  } while (std::chrono::steady_clock::now() < until);
  throw std::runtime_error("Optional reference blocked preview");
}
} // namespace
void LibraryTests(drumfoundry::Json document) {
  using namespace drumfoundry;
  namespace fs = std::filesystem;
  const auto root =
      fs::temp_directory_path() /
      ("drumfoundry-library-" + std::to_string(std::random_device{}()));
  Check(fs::create_directory(root));
  struct Cleanup {
    fs::path p;
    ~Cleanup() { fs::remove_all(p); }
  } cleanup{root};
  const auto library = root / "library",
             folder = library / fs::u8path("Gongs é");
  fs::create_directories(folder);
  const auto sample = folder / "Sample.WAV", settings = root / "library.json";
  Wave(sample);
  Check(analysis::ReadLibraryRoot(settings).empty());
  analysis::SaveLibraryRoot(library, settings);
  Check(analysis::ReadLibraryRoot(settings) == fs::canonical(library));
  const auto relative = analysis::LibraryRelativePath(library, sample);
  Check(analysis::ResolveLibrarySample(library, relative) ==
        library / fs::u8path(relative));
  Check(analysis::LibraryFolder(library).size() == 1);
  Check(
      analysis::LibraryFolder(library, fs::u8path("Gongs é").generic_u8string())
          .size() == 1);
  for (auto path : {"../outside.wav", "/absolute.wav", "C:/absolute.wav",
                    "..\\outside.wav"})
    Reject([&] { analysis::ResolveLibrarySample(library, path); });
  Reject([&] { analysis::LibraryRelativePath(library, root / "outside.wav"); });
  Reject([&] { analysis::SaveLibraryRoot(root / "missing", settings); });
  Check(analysis::ReadLibraryRoot(settings) == fs::canonical(library));
  auto legacy = analysis::PortableReference(
      {{"cell", {{"url", "/reference/Gongs%20%C3%A9/Sample.WAV"}}}}, library);
  Check(legacy.at("libraryPath") == relative && legacy.at("visible") == true);
  document["reference"] = {{"libraryPath", relative}, {"visible", true}};
  document["controls"]["analysis"]["view"] = {{"renderSeconds", .25}};
  ui::AnalysisPanel panel(settings);
  panel.setBounds(0, 0, 800, 500);
  panel.SetDocument(document);
  Finish(panel);
  Check(panel.ReferenceWarning().empty() &&
        panel.Reference().at("libraryPath") == relative);
  Check(panel.RenderRate() ==
        48000); // A 44.1 kHz reference does not retune the renderer.
  ui::AnalysisView *view = nullptr;
  for (auto *child : panel.children())
    if (auto *v = dynamic_cast<ui::AnalysisView *>(child))
      view = v;
  Check(view && view->HasReference());
  const auto nextSample = folder / "Second.wav";
  Wave(nextSample);
  auto gainDocument = document;
  gainDocument["reference"]["referenceGainDb"] = 6.;
  panel.SetDocument(gainDocument);
  Finish(panel);
  panel.StepReference(1);
  Finish(panel);
  Check(panel.Reference().at("libraryPath") ==
        analysis::LibraryRelativePath(library, nextSample));
  Check(panel.Reference().at("referenceGainDb") == 6.);
  panel.showSpectrogram = false;
  panel.resized();
  Check(!view->isVisible());
  bool referencePlayed = false;
  panel.play = [&](auto pcm, unsigned rate, double gain) {
    Check(!pcm->empty() && rate == 48000 &&
          std::abs(gain - std::pow(10., .3)) < 1e-8);
    referencePlayed = true;
  };
  panel.Play(true);
  Check(referencePlayed); // Hiding the plot does not disable reference audio.
  panel.StepReference(-1);
  Finish(panel);
  Check(panel.Reference().at("libraryPath") == relative);
  panel.showSpectrogram = true;
  panel.SetDocument(document);
  Finish(panel);
  panel.SetReferenceVisible(false);
  Check(panel.Ready() && !view->HasReference() &&
        view->Mode() == ui::Comparison::Model);
  const auto hidden = panel.Reference();
  Check(hidden.at("visible") == false && hidden.at("libraryPath") == relative);
  panel.SetReferenceVisible(true);
  Check(view->HasReference());
  fs::remove(sample);
  panel.SetDocument(document);
  Finish(panel);
  Check(!panel.ReferenceWarning().empty() && !view->HasReference());
  Check(panel.Reference().at("libraryPath") ==
        relative); // Save still works with missing WAV.
  bool played = false;
  panel.play = [&](auto pcm, unsigned rate, double gain) {
    Check(!pcm->empty() && rate == 48000 && gain == 1);
    played = true;
  };
  panel.Play(false);
  Check(played);
  Wave(sample);
  panel.UpdateModel(document);
  Finish(panel);
  Check(panel.ReferenceWarning().empty() && view->HasReference());
  panel.ClearReference();
  Finish(panel);
  Check(panel.Reference().is_null() && panel.ReferenceWarning().empty());
  document.erase("reference");
  panel.SetDocument(document);
  Finish(panel);
  Check(panel.Reference().is_null() && view->Mode() == ui::Comparison::Model);
  analysis::SaveLibraryRoot({}, settings);
  Check(analysis::ReadLibraryRoot(settings).empty());
}
