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

    // The fixed design resolution everything is laid out in. The window can be any size /
    // fullscreen; the view stays this size and is letterboxed into the window, so the UI is
    // resolution-independent and stays crisp (rendered at the window's real pixels).
    static constexpr unsigned DESIGN_W = 1280;
    static constexpr unsigned DESIGN_H = 720;

    // Recreate the window from g_settings (fullscreen at desktop resolution, or a windowed
    // size) and re-letterbox the design view. Deferred via requestVideoApply so it happens at
    // a safe point (top of the loop), not mid event-poll.
    void applyVideoMode();
    void requestVideoApply() { m_pendingVideoApply = true; }

    Player* getPlayer() { return m_player.get(); }
    Database& getDatabase() { return m_database; }
    CareerManager* getCareerManager() { return m_careerManager.get(); }

private:
    void applyLetterbox(unsigned winW, unsigned winH);

    sf::RenderWindow m_window;
    sf::View m_view;
    bool m_pendingVideoApply = false;
    std::shared_ptr<Screen> m_currentScreen;
    std::shared_ptr<Screen> m_pendingScreen;
    std::unique_ptr<Player> m_player;
    Database m_database;
    std::unique_ptr<CareerManager> m_careerManager;
};
