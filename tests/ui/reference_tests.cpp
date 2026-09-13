#include "workbench/analysis/catalog.hpp"
#include "workbench/analysis/worker.hpp"
#include <chrono>
#include <fstream>
#include <stdexcept>
namespace {
void Check(bool condition) {
  if (!condition)
    throw std::runtime_error("Reference I/O regression");
}
void Little(std::ostream &out, unsigned value, unsigned bytes) {
  for (unsigned i = 0; i < bytes; ++i)
    out.put(char(value >> (8 * i)));
}
void Wave(const std::filesystem::path &path) {
  std::ofstream out(path, std::ios::binary);
  out.write("RIFF", 4);
  Little(out, 40, 4);
  out.write("WAVEfmt ", 8);
  Little(out, 16, 4);
  Little(out, 1, 2);
  Little(out, 2, 2);
  Little(out, 48000, 4);
  Little(out, 192000, 4);
  Little(out, 4, 2);
  Little(out, 16, 2);
  out.write("data", 4);
  Little(out, 4, 4);
  Little(out, 16384, 2);
  Little(out, 49152, 2);
}
std::shared_ptr<const drumfoundry::analysis::Result>
Render(drumfoundry::analysis::Worker &worker,
       const drumfoundry::analysis::Request &request) {
  worker.Submit(request);
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (std::chrono::steady_clock::now() < deadline) {
    if (auto result = worker.Take())
      return result;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  throw std::runtime_error("Reference render timed out");
}
} // namespace
void ReferenceTests(const drumfoundry::Json &fit) {
  using namespace drumfoundry::analysis;
  const auto root =
      std::filesystem::temp_directory_path() /
      ("drumfoundry-analysis-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directory(root);
  const auto wav = root / "test.wav", catalogPath = root / "catalog.json";
  Wave(wav);
  Check(ReadWave(wav).samples.at(0) == 0);
  Check(ReadWave(wav, Channel::Left).samples.at(0) == .5f);
  Check(ReadWave(wav, Channel::Right).samples.at(0) == -.5f);
  const auto hash = FileHash(wav);
  Check(hash.size() == 64);
  drumfoundry::Json catalog = {
      {"schema", "triggerfish.drumfoundry.references/v1"},
      {"corpora",
       {{{"id", "fixture"},
         {"name", "Fixture"},
         {"cells",
          {{{"path", "test.wav"}, {"sha256", hash}, {"label", "Test"}}}}}}}};
  {
    std::ofstream out(catalogPath);
    out << catalog;
  }
  Catalog loaded;
  loaded.Load(catalogPath);
  Check(loaded.Find({{"sha256", hash}}) != nullptr);
  Check(loaded.Find({{"cell", drumfoundry::Json::object()}}) == nullptr);
  catalog["corpora"][0]["cells"][0]["path"] = "../escape.wav";
  {
    std::ofstream out(catalogPath);
    out << catalog;
  }
  bool rejected = false;
  try {
    loaded.Load(catalogPath);
  } catch (const std::exception &) {
    rejected = true;
  }
  Check(rejected && loaded.Cells().size() == 1);
  {
    Worker worker;
    Request request;
    request.document = fit;
    request.reference = wav;
    request.expectedHash = "wrong";
    request.duration = .01;
    worker.Submit(request);
    std::shared_ptr<const Result> result;
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!(result = worker.Take()) &&
           std::chrono::steady_clock::now() < deadline)
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    Check(result && result->error.find("SHA256") != std::string::npos);
    request.expectedHash = hash;
    for (auto channel : {Channel::Left, Channel::Right}) {
      request.channel = channel;
      for (unsigned rate : {48000u, 96000u}) {
        request.auditionRate = rate;
        const auto first = Render(worker, request),
                   cached = Render(worker, request);
        const auto expected = Resample(ReadWave(wav, channel), rate);
        Check(first->error.empty() && cached->error.empty());
        Check(first->referencePlayback == expected &&
              cached->referencePlayback == expected);
      }
    }
  }
  std::filesystem::remove(wav);
  std::filesystem::remove(catalogPath);
  std::filesystem::remove(root);
}
