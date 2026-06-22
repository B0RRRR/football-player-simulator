#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "Screen.h"

class GameManager {
public:
    GameManager();
    
    // Starts the main game loop
    void run();
    
    // Switches the current active screen
    void changeScreen(std::shared_ptr<Screen> screen);
    
    // Access to window for screens that might need it
    sf::RenderWindow& getWindow() { return m_window; }

private:
    sf::RenderWindow m_window;
    std::shared_ptr<Screen> m_currentScreen;
};
