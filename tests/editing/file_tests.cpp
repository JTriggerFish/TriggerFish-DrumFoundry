#include "editing/files.hpp"
#include <chrono>
#include <stdexcept>
using namespace drumfoundry::editing;
void Require(bool condition) {
  if (!condition)
    throw std::runtime_error("Fit persistence regression");
}
int main(int argc, char **argv) {
  Require(argc == 2);
  const auto document = ReadFit(argv[1]);
  const auto directory =
      std::filesystem::temp_directory_path() /
      ("drumfoundry-file-test-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  Require(std::filesystem::create_directory(directory));
  const auto path = directory / "test.json";
  WriteNewFit(path, document);
  Require(ReadFit(path) == document);
  Require(FitName(path) == document.at("name").get<std::string>());
  bool rejected = false;
  try {
    WriteNewFit(path, document);
  } catch (...) {
    rejected = true;
  }
  Require(rejected && ReadFit(path) == document);
  rejected = false;
  try {
    WriteNewFit(directory / "invalid.json", Json{{"schema", "invalid"}});
  } catch (...) {
    rejected = true;
  }
  Require(rejected && !std::filesystem::exists(directory / "invalid.json"));
  Require(std::filesystem::remove(path));
  Require(std::filesystem::remove(directory));
}
