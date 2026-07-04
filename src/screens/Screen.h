#pragma once
#include <SFML/Graphics.hpp>

class GameManager; // Forward declaration

class Screen {
public:
    virtual ~Screen() = default;
    
    // Initialize resources when the screen is shown
    virtual void init() {}
    
    // Handle input events (mouse, keyboard)
    virtual void handleInput(sf::RenderWindow& window, const sf::Event& event) = 0;
    
    // Update logic (timers, animations)
    virtual void update(sf::Time deltaTime) = 0;
    
    // Draw graphics to the screen
    virtual void draw(sf::RenderWindow& window) = 0;
    
protected:
    GameManager* m_gameManager = nullptr;
    
    friend class GameManager;
    void setGameManager(GameManager* gm) { m_gameManager = gm; }
};
