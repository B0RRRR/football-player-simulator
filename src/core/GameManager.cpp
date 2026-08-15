#include "GameManager.h"
#include "Settings.h"
#include "AssetManager.h"
#include "AudioManager.h"
#include "Logger.h"
#include <typeinfo>
#include "PlatformDisplay.h"
#include <iostream>

GameManager::GameManager()
{
    g_settings.load(); // restore saved settings before the window/audio are created

    m_database.init();
    m_player = std::make_unique<Player>("My Player");
    m_careerManager = std::make_unique<CareerManager>(this);

    applyVideoMode();
    LOG_INFO("Video: " << (g_settings.isFullscreen ? "fullscreen" : "windowed") << " "
             << m_window.getSize().x << "x" << m_window.getSize().y
             << ", difficulty=" << g_settings.difficulty << ", matchSpeed=" << g_settings.matchSpeed);

    // Load global assets here
    AssetManager::get().loadFont("MainFont", "assets/fonts/Roboto-Regular.ttf");
    AudioManager::get().loadAll();
    // Background music. Drop any number of tracks into assets/music/ and they rotate on a loop;
    // if that folder is empty, fall back to a single assets/sounds/menu.* file. Missing files =
    // silence, no crash.
    if (!AudioManager::get().musicPlaylist("assets/music"))
        AudioManager::get().playMusic("menu", true);
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

    // Multisampling smooths every shape edge (circles, rounded panels) across the whole UI.
    sf::ContextSettings ctx;
    ctx.antialiasingLevel = 8;

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
        m_window.create(sf::VideoMode(fw, fh), "Football Career Simulator", sf::Style::None, ctx);
        m_window.setPosition({fx, fy});
    } else {
        m_window.create(sf::VideoMode(g_settings.resWidth, g_settings.resHeight),
                        "Football Career Simulator", sf::Style::Default, ctx);
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
        LOG_INFO("Screen -> " << Log::demangle(typeid(*screen).name()));
        m_pendingScreen = screen;
    }
}

void GameManager::run() {
    sf::Clock clock;
    bool focused = true; // track focus so we don't touch GL while minimised (assume focused at start)

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
            if (event.type == sf::Event::LostFocus)   focused = false;
            if (event.type == sf::Event::GainedFocus) focused = true;
            if (event.type == sf::Event::Resized) {
                applyLetterbox(event.size.width, event.size.height);
            }
            
            // A universal UI click for menu-style screens (gameplay screens opt out).
            if (event.type == sf::Event::MouseButtonPressed
                && event.mouseButton.button == sf::Mouse::Left
                && m_currentScreen && m_currentScreen->playsClickOnPress()) {
                AudioManager::get().sfx("click");
            }

            if (m_currentScreen) {
                m_currentScreen->handleInput(m_window, event);
            }
        }
        
        // Update logic
        AudioManager::get().setMusicEnabled(!m_currentScreen || m_currentScreen->wantsMenuMusic());
        AudioManager::get().setAmbienceEnabled(m_currentScreen && m_currentScreen->wantsMatchAmbience());
        AudioManager::get().update(deltaTime.asSeconds()); // playlists + ambience/reaction fades
        if (m_currentScreen) {
            m_currentScreen->update(deltaTime);
        }
        
        // Draw. While the window is minimised/unfocused (or reports a zero-size surface, as it
        // does when minimised), skip all GL work: on some backends (notably WSLg) swapping buffers
        // on a hidden surface blocks indefinitely, which froze the whole app - the loop never got
        // back to poll the close/restore events. We keep polling events and updating audio above,
        // and just sleep here so it stays responsive and can be restored or closed.
        sf::Vector2u ws = m_window.getSize();
        if (focused && ws.x > 0 && ws.y > 0) {
            m_window.clear(sf::Color::Black);
            if (m_currentScreen) {
                m_currentScreen->draw(m_window);
            }
            m_window.display();
        } else {
            sf::sleep(sf::milliseconds(32)); // no GL while hidden; don't busy-spin either
        }
    }
}
