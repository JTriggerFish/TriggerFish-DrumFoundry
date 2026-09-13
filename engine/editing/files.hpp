#pragma once
#include "document.hpp"
#include <filesystem>
namespace drumfoundry::editing {
Json ReadFit(const std::filesystem::path &);
std::string FitName(const std::filesystem::path &);
// Exclusive creation: existing user fits are never overwritten.
void WriteNewFit(const std::filesystem::path &, const Json &);
std::filesystem::path FitDirectory();
} // namespace drumfoundry::editing
