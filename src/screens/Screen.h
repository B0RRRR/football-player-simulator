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

    // Menu-style screens get an automatic UI click on left-press (handled centrally in
    // GameManager). Gameplay screens (match, training) drive their own audio, so they opt out.
    virtual bool playsClickOnPress() const { return true; }

    // The background music playlist plays on menu screens. A match has its own audio, so it
    // silences the playlist while it's on screen (resumed automatically afterwards).
    virtual bool wantsMenuMusic() const { return true; }

    // Stadium ambience (intershum) plays only during an actual match.
    virtual bool wantsMatchAmbience() const { return false; }

protected:
    GameManager* m_gameManager = nullptr;
    
    friend class GameManager;
    void setGameManager(GameManager* gm) { m_gameManager = gm; }
};
