#include "GameManager.h"
#include "Settings.h"
#include "AssetManager.h"
#include "PlatformDisplay.h"
#include <iostream>

GameManager::GameManager() 
{
    m_database.init();
    m_player = std::make_unique<Player>("My Player");
    m_careerManager = std::make_unique<CareerManager>(this);

    applyVideoMode();

    // Load global assets here
    AssetManager::get().loadFont("MainFont", "assets/fonts/Roboto-Regular.ttf");
}

GameManager::~GameManager() = default;

void GameManager::applyVideoMode() {
    // Remember where the window already was (and its centre) so fullscreen lands on the SAME
    // monitor. Exclusive sf::Style::Fullscreen on a multi-monitor setup switches the display mode
    // and often jumps to the wrong screen, stretching everything - so we avoid it entirely.
    bool hadWindow = m_window.isOpen();
    sf::Vector2i prevPos = hadWindow ? m_window.getPosition() : sf::Vector2i(0, 0);
    sf::Vector2u prevSize = hadWindow ? m_window.getSize()
                                      : sf::Vector2u(g_settings.resWidth, g_settings.resHeight);
    int cx = prevPos.x + (int)prevSize.x / 2;
    int cy = prevPos.y + (int)prevSize.y / 2;

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

    if (g_settings.isFullscreen) {
        // Borderless windowed "fullscreen" on exactly the monitor the window is on. Style::None
        // doesn't change the video mode (no stretching); XRandR gives that monitor's real bounds
        // so different-width monitors work and it never spans both.
        int fx = 0, fy = 0;
        unsigned fw = desktop.width, fh = desktop.height;   // fallback: whole desktop
        int mx, my, mw, mh;
        if (Platform::monitorContaining(cx, cy, mx, my, mw, mh)) {
            fx = mx; fy = my; fw = (unsigned)mw; fh = (unsigned)mh;
        }
        m_window.create(sf::VideoMode(fw, fh), "Football Career Simulator", sf::Style::None);
        m_window.setPosition({fx, fy});
    } else {
        m_window.create(sf::VideoMode(g_settings.resWidth, g_settings.resHeight),
                        "Football Career Simulator", sf::Style::Default);
        if (hadWindow) {
            // Keep the windowed window on the same monitor, near its top-left.
            int wx = 60, wy = 60;
            int mx, my, mw, mh;
            if (Platform::monitorContaining(cx, cy, mx, my, mw, mh)) { wx = mx + 60; wy = my + 60; }
            m_window.setPosition({wx, wy});
        }
    }
    m_window.setFramerateLimit(60);

    // The view is ALWAYS the design size; only the letterbox viewport changes with the window.
    m_view.setSize((float)DESIGN_W, (float)DESIGN_H);
    m_view.setCenter(DESIGN_W / 2.f, DESIGN_H / 2.f);
    applyLetterbox(m_window.getSize().x, m_window.getSize().y);
}

void GameManager::applyLetterbox(unsigned winW, unsigned winH) {
    if (winH == 0) return;
    float windowRatio = winW / (float)winH;
    float viewRatio = DESIGN_W / (float)DESIGN_H;
    float sizeX = 1.f, sizeY = 1.f, posX = 0.f, posY = 0.f;
    if (windowRatio >= viewRatio) { sizeX = viewRatio / windowRatio; posX = (1.f - sizeX) / 2.f; }
    else                          { sizeY = windowRatio / viewRatio; posY = (1.f - sizeY) / 2.f; }
    m_view.setViewport(sf::FloatRect(posX, posY, sizeX, sizeY));
    m_window.setView(m_view);

    // On-screen pixels per design unit: the design view (DESIGN_H tall) maps onto this many real
    // pixels. UIKit reads it to keep text crisp when the canvas is scaled up to fill the screen.
    g_renderScale = (winH * sizeY) / (float)DESIGN_H;
    if (g_renderScale < 0.1f) g_renderScale = 1.f;
}


void GameManager::changeScreen(std::shared_ptr<Screen> screen) {
    if (screen) {
        m_pendingScreen = screen;
    }
}

void GameManager::run() {
    sf::Clock clock;
    
    while (m_window.isOpen()) {
        if (m_pendingVideoApply) {
            m_pendingVideoApply = false;
            applyVideoMode();        // recreate window at a safe point, not mid event-poll
            clock.restart();         // the create() stall shouldn't count as a huge dt
        }
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
                applyLetterbox(event.size.width, event.size.height);
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
