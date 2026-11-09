// texture_manager.cpp
#include "core/texture_manager.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace
{
constexpr unsigned int kPlaceholderSize = 64;
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
    sf::Image image(sf::Vector2u{kPlaceholderSize, kPlaceholderSize}, sf::Color::Transparent);

    if (color.a > 0)
    {
        const float half = static_cast<float>(kPlaceholderSize) / 2.f;
        for (unsigned int y = 0; y < kPlaceholderSize; ++y)
        {
            for (unsigned int x = 0; x < kPlaceholderSize; ++x)
            {
                const float dx = std::abs(static_cast<float>(x) + 0.5f - half);
                const float dy = std::abs(static_cast<float>(y) + 0.5f - half);

                // Fill a 2:1 isometric diamond so placeholders line up with the
                // real tiles instead of looking like squares.
                if ((dx / half) + (dy / half) <= 1.f)
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
