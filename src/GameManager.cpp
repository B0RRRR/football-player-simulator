#include "GameManager.h"
#include "Settings.h"
#include "AssetManager.h"
#include <iostream>

GameManager::GameManager() 
{
    m_database.init();
    m_player = new Player("My Player");
    m_careerManager = new CareerManager(this);

    // Initialize window based on settings
    int style = g_settings.isFullscreen ? sf::Style::Fullscreen : sf::Style::Default;
    m_window.create(sf::VideoMode(g_settings.resWidth, g_settings.resHeight), "Football Career Simulator", style);
    m_window.setFramerateLimit(60);
    
    // Load global assets here
    AssetManager::get().loadFont("MainFont", "assets/fonts/Roboto-Regular.ttf");
}

GameManager::~GameManager() {
    delete m_careerManager;
    delete m_player;
}


void GameManager::changeScreen(std::shared_ptr<Screen> screen) {
    if (screen) {
        m_currentScreen = screen;
        m_currentScreen->setGameManager(this);
        m_currentScreen->init();
    }
}

void GameManager::run() {
    sf::Clock clock;
    
    while (m_window.isOpen()) {
        sf::Time deltaTime = clock.restart();
        if (deltaTime.asSeconds() > 0.1f) {
            deltaTime = sf::seconds(0.1f); // Cap delta time to prevent physics explosions and freezing after resuming
        }
        
        // Handle events
        sf::Event event;
        while (m_window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                m_window.close();
            }
            
            if (m_currentScreen) {
                m_currentScreen->handleInput(m_window, event);
            }
        }
        
        // Update logic
        if (m_currentScreen) {
            m_currentScreen->update(deltaTime);
        }
        
        // Draw
        m_window.clear(sf::Color::Black);
        
        if (m_currentScreen) {
            m_currentScreen->draw(m_window);
        }
        
        m_window.display();
    }
}
