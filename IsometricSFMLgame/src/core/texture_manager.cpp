// texture_manager.cpp
#include "core/texture_manager.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "core/constants.hpp"

namespace
{
// Placeholders use the same 2:1 footprint as the map grid (64x32 by default).
// A square texture would make every tile twice as tall as the isometric cell
// it represents, so the diamonds would overlap and no longer line up with the
// grid lines or with real 2:1 tile art.
constexpr unsigned int kPlaceholderWidth = config::tileWidth;
constexpr unsigned int kPlaceholderHeight = config::tileHeight;
} // namespace

bool TextureManager::loadTexture(const std::string& name, const std::filesystem::path& filename)
{
    if (filename.empty())
        return false;

    sf::Texture texture;
    if (!texture.loadFromFile(filename))
    {
        std::cerr << "[textures] could not load '" << filename.string() << "'\n";
        return false;
    }

    textures_[name] = std::move(texture);
    return true;
}

void TextureManager::createPlaceholder(const std::string& name, sf::Color color)
{
    sf::Image image(sf::Vector2u{kPlaceholderWidth, kPlaceholderHeight}, sf::Color::Transparent);

    if (color.a > 0)
    {
        const float halfWidth = static_cast<float>(kPlaceholderWidth) / 2.f;
        const float halfHeight = static_cast<float>(kPlaceholderHeight) / 2.f;
        for (unsigned int y = 0; y < kPlaceholderHeight; ++y)
        {
            for (unsigned int x = 0; x < kPlaceholderWidth; ++x)
            {
                const float dx = std::abs(static_cast<float>(x) + 0.5f - halfWidth);
                const float dy = std::abs(static_cast<float>(y) + 0.5f - halfHeight);

                // Fill a 2:1 isometric diamond so placeholders line up with the
                // real tiles instead of looking like squares.
                if ((dx / halfWidth) + (dy / halfHeight) <= 1.f)
                    image.setPixel(sf::Vector2u{x, y}, color);
            }
        }
    }

    sf::Texture texture;
    if (!texture.loadFromImage(image))
        throw std::runtime_error("Failed to create placeholder texture: " + name);

    textures_[name] = std::move(texture);
}

bool TextureManager::has(const std::string& name) const
{
    return textures_.find(name) != textures_.end();
}

const sf::Texture& TextureManager::getRef(const std::string& name) const
{
    return textures_.at(name);
}
