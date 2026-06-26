#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <memory>
#include "MatchEngine.h"

class MatchScreen : public Screen {
public:
    MatchScreen();
    ~MatchScreen() = default;
    virtual void init() override;
    virtual void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    virtual void update(sf::Time deltaTime) override;
    virtual void draw(sf::RenderWindow& window) override;

private:
    void initMinigame();
    void updateMinigame(sf::Time deltaTime);

    std::shared_ptr<MatchEngine> m_engine;
    
    sf::Text m_scoreText;
    sf::Text m_timeText;
    sf::Text m_logText;
    sf::Text m_statusText;
    std::vector<std::string> m_visibleLogs;
    
    sf::RectangleShape m_pitchRect;
    
    // Minigame variables
    sf::CircleShape m_playerSprite;
    sf::CircleShape m_ballSprite;
    sf::CircleShape m_targetSprite; // E.g., goal or teammate
    sf::CircleShape m_enemySprite;
    
    bool m_minigameActive;
    float m_minigameTimer;
    
    // New dynamic minigame variables
    float m_targetDir;
    float m_enemyDir;
    float m_enemySpeed;
    sf::Text m_promptText;
    
    float m_simTimer;
};
