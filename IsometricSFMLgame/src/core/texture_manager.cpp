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

void TextureManager::createPlayerPlaceholder(const std::string& name, sf::Color color)
{
    const unsigned int frameCount = config::playerPlaceholderFrames;
    const unsigned int frameSize = config::playerPlaceholderFrameSize;

    sf::Image image(sf::Vector2u{frameSize * frameCount, frameSize}, sf::Color::Transparent);

    const sf::Vector2u imageSize = image.getSize();
    const auto paint = [&image, imageSize, color](int x, int y)
    {
        if (x < 0 || y < 0)
            return;

        const auto position = sf::Vector2u{static_cast<unsigned int>(x), static_cast<unsigned int>(y)};
        if (position.x < imageSize.x && position.y < imageSize.y)
            image.setPixel(position, color);
    };

    const int centreX = static_cast<int>(frameSize) / 2;
    const int feetY = static_cast<int>(frameSize) - 3;

    for (unsigned int frame = 0; frame < frameCount; ++frame)
    {
        const int frameX = static_cast<int>(frame * frameSize);

        // Two idle poses, alternating every frame: the figure bobs up 2 px.
        const int bob = (frame % 2 == 0) ? 0 : 2;
        const int bodyTop = feetY - 22 - bob;
        const int headCentreY = bodyTop - 7;

        for (int y = bodyTop; y <= feetY - bob; ++y)
            for (int x = centreX - 7; x <= centreX + 7; ++x)
                paint(frameX + x, y);

        // Round head (radius 6) above the body.
        for (int y = -6; y <= 6; ++y)
        {
            const int halfWidth = static_cast<int>(std::sqrt(36 - y * y));
            for (int x = -halfWidth; x <= halfWidth; ++x)
                paint(frameX + centreX + x, headCentreY + y);
        }
    }

    sf::Texture texture;
    if (!texture.loadFromImage(image))
        throw std::runtime_error("Failed to create player placeholder texture: " + name);

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
