//// tile_preview.hpp
//#pragma once
//
//#include "map.hpp"
//#include <SFML/Graphics.hpp>
//
//class TilePreview {
//private:
//    Map& map;
//    std::optional<sf::Sprite> previewSprite; // Using optional since we need to initialize later
//    sf::Vector3i currentGridPos;
//    bool isValid;
//
//public:
//    TilePreview(Map& mapRef);
//
//    void update(const sf::Vector3i& gridPos, TileType type, const sf::Texture& texture);
//    void draw(sf::RenderWindow& window);
//    bool isValidPlacement() const { return isValid; }
//};