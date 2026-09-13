#pragma once
#include "runtime/voice.hpp"
#include <filesystem>
namespace drumfoundry::analysis {
struct ReferenceCell {
  std::string corpus, corpusName;
  Json metadata;
  std::filesystem::path path;
  double gainDb{};
};
class Catalog {
public:
  void Load(const std::filesystem::path &);
  const std::vector<ReferenceCell> &Cells() const { return cells_; }
  const ReferenceCell *Find(const Json &reference) const;
private:
  std::vector<ReferenceCell> cells_;
};
} // namespace drumfoundry::analysis
