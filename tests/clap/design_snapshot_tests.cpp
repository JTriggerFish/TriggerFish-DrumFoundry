#include "adapters/clap/plugin.hpp"
#include <stdexcept>
#include <thread>

// Exercise the real saved-state path concurrently with host automation. The
// publisher switches between two valid curves whose mixed values are invalid.
void DesignSnapshotTests() {
  using namespace drumfoundry;
  using namespace clap_adapter;
  clap_host_t host{CLAP_VERSION,  nullptr, "Snapshot test",
                   "TriggerFish", "",      "1"};
  auto p = std::make_unique<Plugin>(&host);
  if (!p->Init())
    throw std::runtime_error("Snapshot init");
  p->SelectFactory(5);
  const auto id = [](const char *key) {
    for (const auto &parameter : DesignParameters())
      if (parameter.recipe == detail::Recipe::MetallicPlate &&
          parameter.descriptor->key == key)
        return parameter.id;
    throw std::runtime_error("Missing curve ID");
  };
  const auto knot = id("body_decay_frequency_1");
  const auto upper = id("body_decay_frequency_7");
  p->SetParameter(id("body_decay_active_1"), 0);
  p->SetParameter(knot, 10000);
  p->SetParameter(upper, 15000);
  p->SetParameter(id("body_decay_active_1"), 1);
  std::atomic<bool> stop{}, started{};
  std::thread writer([&] {
    started = true;
    while (!stop.load()) {
      p->SetParameter(upper, 20000);
      p->SetParameter(knot, 19000);
      p->SetParameter(knot, 10000);
      p->SetParameter(upper, 15000);
    }
  });
  bool valid = true;
  try {
    while (!started.load())
      std::this_thread::yield();
    for (unsigned i = 0; i < 300; ++i) {
      std::string saved;
      clap_ostream_t stream{&saved,
                            [](const clap_ostream_t *s, const void *data,
                               uint64_t size) -> int64_t {
                              static_cast<std::string *>(s->ctx)->append(
                                  static_cast<const char *>(data), size);
                              return size;
                            }};
      valid &= p->Save(&stream);
      const auto document = Json::parse(saved).at("document");
      for (const auto &node : document.at("instrument").at("nodes")) {
        const auto &v = node.at("parameters");
        if (v.contains("body_decay_frequency_7"))
          valid &= v.at("body_decay_frequency_1").get<double>() <=
                   v.at("body_decay_frequency_7").get<double>();
      }
      if (i % 50 == 0) {
        Voice reloaded(48000, document);
      }
    }
  } catch (...) {
    stop = true;
    writer.join();
    throw;
  }
  stop = true;
  writer.join();
  if (!valid)
    throw std::runtime_error("Saving automation captured a torn T60 curve");
}
