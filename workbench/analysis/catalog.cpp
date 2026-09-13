#include "catalog.hpp"
#include <fstream>
#include <stdexcept>
namespace drumfoundry::analysis {
void Catalog::Load(const std::filesystem::path &path) {
  if (std::filesystem::file_size(path) > 1024 * 1024)
    throw std::runtime_error("Reference catalogue exceeds 1 MB");
  std::ifstream input(path);
  if (!input)
    throw std::runtime_error("Cannot open reference catalogue");
  const auto document = Json::parse(input);
  if (document.at("schema") != "triggerfish.drumfoundry.references/v1")
    throw std::runtime_error("Unsupported reference catalogue schema");
  std::vector<ReferenceCell> cells;
  const auto root = std::filesystem::canonical(path.parent_path());
  for (const auto &corpus : document.at("corpora"))
    for (const auto &cell : corpus.at("cells")) {
      const auto relative =
          std::filesystem::u8path(cell.at("path").get<std::string>());
      if (relative.is_absolute())
        throw std::runtime_error("Reference catalogue paths must be relative");
      const auto resolved = std::filesystem::weakly_canonical(root / relative);
      const auto within = resolved.lexically_relative(root);
      if (within.empty() || *within.begin() == "..")
        throw std::runtime_error("Reference path escapes its catalogue folder");
      cells.push_back({corpus.at("id"), corpus.at("name"), cell, resolved,
                       corpus.value("audition_trim_db", 0.)});
    }
  cells_ = std::move(cells);
}
const ReferenceCell *Catalog::Find(const Json &reference) const {
  if (!reference.is_object())
    return nullptr;
  for (const auto &cell : cells_) {
    if (cell.metadata.value("sha256", "") == reference.value("sha256", "") &&
        !reference.value("sha256", "").empty())
      return &cell;
    if (reference.contains("cell") && reference.at("cell").is_object()) {
      const auto url = reference.at("cell").value("url", "");
      if (!url.empty() && url == cell.metadata.value("url", ""))
        return &cell;
    }
  }
  return nullptr;
}
} // namespace drumfoundry::analysis
