// texture_manager.hpp
#pragma once

#include <SFML/Graphics.hpp>

#include <filesystem>
#include <map>
#include <string>

// Owns every sf::Texture used by the game and guarantees the textures stay
// alive for as long as sprites reference them.
class TextureManager
{
public:
    // Loads `filename` under `name`. Returns false when the file is missing or
    // invalid; the caller can then call createPlaceholder().
    bool loadTexture(const std::string& name, const std::filesystem::path& filename);

    // Creates an isometric diamond placeholder. This keeps the game playable
    // when the art files are not present (the repository ships without them).
    void createPlaceholder(const std::string& name, sf::Color color);

    [[nodiscard]] bool has(const std::string& name) const;

    // Returns the texture stored under `name`.
    // Throws std::out_of_range when the name is unknown.
    [[nodiscard]] const sf::Texture& getRef(const std::string& name) const;

private:
    std::map<std::string, sf::Texture> textures_;
};
