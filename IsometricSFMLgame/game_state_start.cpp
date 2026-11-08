#include "game_state_start.hpp"
#include "game_state_editor.hpp"

GameStateStart::GameStateStart(Game* game) : GameState(game)
{
    // Size and center the view to match the window.
    const sf::Vector2f size = static_cast<sf::Vector2f>(this->game->window.getSize());
    this->view.setSize(size);
    this->view.setCenter(size * 0.5f);
}

void GameStateStart::draw(const float /*dt*/)
{
    if (this->game)
    {
        this->game->window.setView(this->view);
        this->game->window.clear(sf::Color::Black);
		// Drawing the Start Button // REMOVE THIS AND REPLACE WITH UI MANAGER WHEN ITS DONE
        
    }
}

void GameStateStart::update(const float /*dt*/)
{
    // Update logic for the start state
}

void GameStateStart::loadgame()
{
    if (this->game)
    {
        // Replace the start state with the editor so the stack doesn't grow
        // every time Space is pressed.
        this->game->changeState(new GameStateEditor(this->game));
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
