// texture_manager.cpp
#include "core/texture_manager.hpp"

#include <algorithm>
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

// Drawing helpers for the generated player art. They clip to the image, so the
// shapes can be described with plain pixel coordinates and no bounds checks.
void fillRect(sf::Image& image, int left, int top, int width, int height, sf::Color color)
{
    const sf::Vector2u imageSize = image.getSize();

    for (int y = std::max(0, top); y < top + height && y < static_cast<int>(imageSize.y); ++y)
    {
        for (int x = std::max(0, left); x < left + width && x < static_cast<int>(imageSize.x); ++x)
        {
            image.setPixel(sf::Vector2u{static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, color);
        }
    }
}

void fillCircle(sf::Image& image, int centerX, int centerY, int radius, sf::Color color)
{
    for (int offsetY = -radius; offsetY <= radius; ++offsetY)
    {
        const int halfWidth = static_cast<int>(std::sqrt(static_cast<double>(radius * radius - offsetY * offsetY)));
        fillRect(image, centerX - halfWidth, centerY + offsetY, halfWidth * 2 + 1, 1, color);
    }
}
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
    constexpr unsigned int frameCount = config::playerPlaceholderFrames;
    constexpr unsigned int frameSize = config::playerPlaceholderFrameSize;

    sf::Image image(sf::Vector2u{frameSize * frameCount, frameSize}, sf::Color::Transparent);

    // Frame layout (48x48, frame-local coordinates):
    //   - body: a 15 px wide bar, 22 px tall, standing on the feet line
    //   - head: a filled circle of radius 6, sitting just above the body
    //   - feetY: 3 px above the bottom edge of the frame
    constexpr int bodyHalfWidth = 7;
    constexpr int headRadius = 6;

    const int centreX = static_cast<int>(frameSize) / 2;
    const int feetY = static_cast<int>(frameSize) - 3;

    for (unsigned int frame = 0; frame < frameCount; ++frame)
    {
        const int frameX = static_cast<int>(frame * frameSize);

        // Two idle poses, alternating every frame: the figure bobs up 2 px.
        const int bob = (frame % 2 == 0) ? 0 : 2;

        const int bodyBottom = feetY - bob;
        const int bodyTop = bodyBottom - 22;

        fillRect(image, frameX + centreX - bodyHalfWidth, bodyTop, bodyHalfWidth * 2 + 1, bodyBottom - bodyTop + 1,
                 color);
        fillCircle(image, frameX + centreX, bodyTop - headRadius - 1, headRadius, color);
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
