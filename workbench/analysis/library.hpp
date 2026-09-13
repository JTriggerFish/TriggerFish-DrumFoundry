#pragma once
#include "runtime/voice.hpp"
#include <filesystem>

namespace drumfoundry::analysis {
// Local preference only. Never place this absolute root into a preset.
std::filesystem::path LibrarySettingsPath();
std::filesystem::path
ReadLibraryRoot(const std::filesystem::path &settings = LibrarySettingsPath());
void SaveLibraryRoot(
    const std::filesystem::path &root,
    const std::filesystem::path &settings = LibrarySettingsPath());
// Resolve portable paths inside the configured root, including symlink
// boundaries.
std::filesystem::path ResolveLibrarySample(const std::filesystem::path &root,
                                           const std::string &relative);
std::string LibraryRelativePath(const std::filesystem::path &root,
                                const std::filesystem::path &sample);
// One folder at a time: no recursive scan or decoding when opening the menu.
std::vector<std::filesystem::directory_entry>
LibraryFolder(const std::filesystem::path &root,
              const std::string &relative = "");
// Convert old URL/catalogue or absolute-path metadata, without changing the
// sound.
Json PortableReference(Json reference, const std::filesystem::path &root);
} // namespace drumfoundry::analysis
