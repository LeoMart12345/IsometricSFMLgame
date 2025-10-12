#include "game_state_start.hpp"
#include "game_state_editor.hpp" // Add this include to fix the error
#include <iostream>

GameStateStart::GameStateStart(Game* game) : GameState(game)
{
    // Initialize any resources for the start state
 

}

void GameStateStart::draw(const float dt)
{
    if (this->game)
    {
        this->game->window.setView(this->view);
        this->game->window.clear(sf::Color::Black);
		// Drawing the Start Button // REMOVE THIS AND REPLACE WITH UI MANAGER WHEN ITS DONE
        
    }
}

void GameStateStart::update(const float dt)
{
    // Update logic for the start state
}

void GameStateStart::loadgame()
{
    if (this->game)
    {
        this->game->pushState(new GameStateEditor(this->game));
    }
} 

void GameStateStart::handleInput()
{
    if (this->game)
    {
        while (std::optional<sf::Event> event = this->game->window.pollEvent())
        {
            if (event)
            {
                if (event->is<sf::Event::Closed>())
                {
                    this->game->window.close();
                }
                else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
                {
                    if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
                    {
                        this->game->window.close();
                    }
                    else if (keyPressed->scancode == sf::Keyboard::Scancode::Space)
                    {
                        this->loadgame(); // Transition to the editor state
                    }
                }
            }
        }
    }
}