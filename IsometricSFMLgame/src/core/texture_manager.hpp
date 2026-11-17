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

    // Creates a 2:1 isometric diamond placeholder (config::tileWidth by
    // config::tileHeight). This keeps the game playable when the art files are
    // not present (the repository ships without them) and matches the footprint
    // that Tile and Map expect from real tile art.
    void createPlaceholder(const std::string& name, sf::Color color);

    // Creates the stand-in idle strip used when assets/player.png is missing:
    // config::playerPlaceholderFrames square frames of a figure that bobs up
    // and down, so the animation is visible before any real art is supplied.
    void createPlayerPlaceholder(const std::string& name, sf::Color color);

    // Returns the texture stored under `name`.
    // Throws std::out_of_range when the name is unknown.
    [[nodiscard]] const sf::Texture& getRef(const std::string& name) const;

private:
    // Uploads a generated image and stores it under `name`.
    void store(const std::string& name, const sf::Image& image);

    std::map<std::string, sf::Texture> textures_;
};
