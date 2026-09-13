#include "adapters/clap/plugin.hpp"
#include <atomic>
#include <stdexcept>
#include <thread>
using namespace drumfoundry;
using namespace drumfoundry::clap_adapter;
namespace {
void Check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
struct Output {
  std::string text;
  clap_ostream_t stream{
      this,
      [](const clap_ostream_t *s, const void *data, uint64_t size) -> int64_t {
        static_cast<Output *>(s->ctx)->text.append(
            static_cast<const char *>(data), std::size_t(size));
        return int64_t(size);
      }};
};
void RawPatches(Plugin &plugin) {
  for (unsigned i = 0; i < unsigned(detail::Recipe::Count); ++i) {
    const auto key =
        Topology(detail::Recipe(i)).at("recipe").get<std::string>();
    const auto patch = DefaultPatch(key);
    plugin.EditDocument(patch);
    const auto document = plugin.EditableDocument();
    Check(document.at("instrument") == patch &&
              document.at("reference").is_null(),
          "Raw patch preserved with explicit metadata");
    Check(WithFitEnvelope(document) == document,
          "Existing fit is not rewrapped");
    Voice raw(48000, patch), fit(48000, document);
    raw.Trigger(raw.Event());
    fit.Trigger(fit.Event());
    std::vector<float> a(4096), b(4096);
    raw.Process(a.data(), a.size());
    fit.Process(b.data(), b.size());
    Check(a == b, "Raw patch import must not change audio");
  }
  const auto before = plugin.EditableDocument();
  auto bad = DefaultPatch("drum.kick.v1");
  bad["name"] = 42;
  bool rejected = false;
  try {
    plugin.EditDocument(bad);
  } catch (const std::exception &) {
    rejected = true;
  }
  Check(rejected && plugin.EditableDocument() == before,
        "Invalid import leaves host document intact");
}
void CoherentSave(Plugin &plugin) {
  std::array<Json, 6> expected;
  for (int i = 0; i < 6; ++i) {
    plugin.SetParameter(Preset, i);
    expected[i] = plugin.EditableDocument();
  }
  std::atomic<bool> stop{};
  std::thread selection([&] {
    unsigned index = 0;
    while (!stop) {
      plugin.SetParameter(Preset, index++ % 6);
      std::this_thread::yield();
    }
  });
  bool coherent = true;
  try {
    for (unsigned i = 0; i < 300; ++i) {
      Output out;
      Check(plugin.Save(&out.stream), "State save failed");
      const auto state = Json::parse(out.text);
      const int preset = state.at("parameters").at("100");
      coherent &= state.at("document") == expected.at(preset);
    }
  } catch (...) {
    stop = true;
    selection.join();
    throw;
  }
  stop = true;
  selection.join();
  Check(coherent, "Saved document and preset controls come from one selection");
}
} // namespace
void DocumentHostTests() {
  clap_host_t host{CLAP_VERSION, nullptr, "Test", "TriggerFish", "", "1"};
  Plugin plugin(&host);
  Check(plugin.Init(), "Initialize host test");
  CoherentSave(plugin);
  RawPatches(plugin);
  plugin.SelectFactory(5);
  const auto factory = plugin.EditableDocument();
  auto edited = factory;
  edited["controls"]["event"]["hardness"] = .123;
  plugin.EditDocument(edited);
  plugin.SelectFactory(5);
  Check(plugin.Value(Preset) == 5 && plugin.EditableDocument() == factory,
        "Selecting the current factory preset must discard custom edits");
}
