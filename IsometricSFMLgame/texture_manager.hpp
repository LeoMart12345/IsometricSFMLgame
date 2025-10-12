// texture_manager.hpp
#pragma once

#include <SFML/Graphics.hpp>
#include <map>
#include <string>

class TextureManager
{
private:
    std::map<std::string, sf::Texture> textures;

public:
    void loadTexture(const std::string& name, const std::string& filename);
    sf::Texture& getRef(const std::string& texture);
};