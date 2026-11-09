// asset_path.hpp
#pragma once

#include <filesystem>
#include <string>

// Resolves the paths of the game's art files.
//
// Visual Studio runs the executable from a different working directory than
// people usually expect, so a plain relative path such as "assets/grass.png"
// only works when the process happens to be started from the repository root.
// These helpers additionally look next to the executable (and a few directories
// above it), which covers Visual Studio's output folders as well as a CMake
// build directory.
namespace assets
{
// Remembers where the executable lives. Call once from main() with argv[0].
void initialize(const char* executablePath);

// Returns the first existing candidate for `relativePath`, or `relativePath`
// itself when nothing was found (so loaders can report a useful error).
std::filesystem::path resolve(const std::string& relativePath);
} // namespace assets
