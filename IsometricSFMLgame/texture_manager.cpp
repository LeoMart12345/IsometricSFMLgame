#include "texture_manager.hpp"
#include <stdexcept>

void TextureManager::loadTexture(const std::string& name, const std::string& filename)
{
    sf::Texture tex;
    if (!tex.loadFromFile(filename))
        throw std::runtime_error("Failed to load texture: " + filename);

    this->textures[name] = tex;
}

sf::Texture& TextureManager::getRef(const std::string& texture)
{
    return this->textures.at(texture); // Throws std::out_of_range if not found
}
