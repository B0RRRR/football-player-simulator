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
    
    m_view.setSize(g_settings.resWidth, g_settings.resHeight);
    m_view.setCenter(g_settings.resWidth / 2.f, g_settings.resHeight / 2.f);
    m_window.setView(m_view);
    
    // Load global assets here
    AssetManager::get().loadFont("MainFont", "assets/fonts/Roboto-Regular.ttf");
}

GameManager::~GameManager() {
    delete m_careerManager;
    delete m_player;
}


void GameManager::changeScreen(std::shared_ptr<Screen> screen) {
    if (screen) {
        m_pendingScreen = screen;
    }
}

void GameManager::run() {
    sf::Clock clock;
    
    while (m_window.isOpen()) {
        if (m_pendingScreen) {
            m_currentScreen = m_pendingScreen;
            m_pendingScreen = nullptr;
            m_currentScreen->setGameManager(this);
            m_currentScreen->init();
        }
        
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
            if (event.type == sf::Event::Resized) {
                // Calculate letterbox view
                float windowRatio = event.size.width / (float)event.size.height;
                float viewRatio = g_settings.resWidth / (float)g_settings.resHeight;
                float sizeX = 1.0f;
                float sizeY = 1.0f;
                float posX = 0.0f;
                float posY = 0.0f;

                bool horizontalSpacing = true;
                if (windowRatio < viewRatio) horizontalSpacing = false;

                if (horizontalSpacing) {
                    sizeX = viewRatio / windowRatio;
                    posX = (1.0f - sizeX) / 2.0f;
                } else {
                    sizeY = windowRatio / viewRatio;
                    posY = (1.0f - sizeY) / 2.0f;
                }

                m_view.setViewport(sf::FloatRect(posX, posY, sizeX, sizeY));
                m_window.setView(m_view);
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
