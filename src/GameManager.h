#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "Screen.h"
#include "Player.h"
#include "Database.h"
#include "CareerManager.h"

class GameManager {
public:
    GameManager();
    ~GameManager();
    
    // Starts the main game loop
    void run();
    
    // Switches the current active screen
    void changeScreen(std::shared_ptr<Screen> screen);
    
    // Access to window for screens that might need it
    sf::RenderWindow& getWindow() { return m_window; }

    Player* getPlayer() { return m_player; }
    Database& getDatabase() { return m_database; }
    CareerManager* getCareerManager() { return m_careerManager; }

private:
    sf::RenderWindow m_window;
    sf::View m_view;
    std::shared_ptr<Screen> m_currentScreen;
    Player* m_player;
    Database m_database;
    CareerManager* m_careerManager;
};
