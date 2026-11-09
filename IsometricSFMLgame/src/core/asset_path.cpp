// asset_path.cpp
#include "core/asset_path.hpp"

#include <system_error>
#include <vector>

namespace
{
std::filesystem::path g_executableDirectory;

// How many parent directories above the executable are searched. Three levels
// covers <repo>/build/bin/<config>/<exe> and Visual Studio's x64/Debug layout.
constexpr int kMaxParentLevels = 4;
} // namespace

void assets::initialize(const char* executablePath)
{
    if (executablePath == nullptr)
        return;

    std::error_code error;
    const std::filesystem::path absolute = std::filesystem::absolute(executablePath, error);
    if (!error)
        g_executableDirectory = absolute.parent_path();
}

std::filesystem::path assets::resolve(const std::string& relativePath)
{
    std::vector<std::filesystem::path> candidates;
    candidates.emplace_back(relativePath); // relative to the working directory

    if (!g_executableDirectory.empty())
    {
        std::filesystem::path base = g_executableDirectory;
        for (int level = 0; level < kMaxParentLevels && !base.empty(); ++level)
        {
            candidates.push_back(base / relativePath);
            base = base.parent_path();
        }
    }

    std::error_code error;
    for (const std::filesystem::path& candidate : candidates)
    {
        if (std::filesystem::exists(candidate, error) && !error)
            return candidate;
        error.clear();
    }

    return std::filesystem::path(relativePath);
}
