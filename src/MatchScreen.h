#pragma once
#include "Screen.h"
#include "MatchEngine.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

enum class VisualState {
    Kickoff,
    NormalPlay,
    Attacking,
    GoalCelebration,
    WaitingForMinigame,
    GoalKick
};

struct PlayerDot {
    sf::CircleShape shape;
    sf::Vector2f targetPos;
    float speed;
    bool isHome;
};

class MatchScreen : public Screen {
public:
    MatchScreen();
    virtual void init() override;
    virtual void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    virtual void update(sf::Time deltaTime) override;
    virtual void draw(sf::RenderWindow& window) override;

private:
    void initMinigame();
    void updateMinigame(sf::Time deltaTime);
    void updateVisuals(sf::Time deltaTime);
    void resetToKickoff();

    std::shared_ptr<MatchEngine> m_engine;
    
    // UI Elements
    sf::Sprite m_homeLogo;
    sf::Sprite m_awayLogo;
    sf::Text m_homeName;
    sf::Text m_awayName;
    sf::Text m_scoreText;
    sf::Text m_timeText;
    
    sf::Text m_logText;
    sf::Text m_statusText;
    sf::Text m_statsTitle;
    sf::Text m_homeStatsText;
    sf::Text m_awayStatsText;
    
    std::vector<MatchEvent> m_visibleLogs;
    std::vector<sf::RectangleShape> m_momentumBars;
    
    // 2D Pitch Elements
    sf::RectangleShape m_pitchRect;
    sf::RectangleShape m_pitchLines;
    sf::CircleShape m_pitchCenter;
    sf::RectangleShape m_leftGoal;
    sf::RectangleShape m_rightGoal;
    
    std::vector<PlayerDot> m_dots;
    sf::CircleShape m_visualBall;
    sf::Vector2f m_ballTarget;
    int m_ballCarrierIdx;
    
    VisualState m_visualState;
    float m_stateTimer;
    MatchEvent m_pendingEvent;
    
    float m_simTimer;
    
    // Minigame Elements
    bool m_minigameActive;
    float m_minigameTimer;
    sf::RectangleShape m_minigameOverlay;
    sf::CircleShape m_playerSprite;
    sf::CircleShape m_enemySprite;
    sf::CircleShape m_ballSprite;
    sf::CircleShape m_targetSprite;
    sf::Text m_promptText;
    
    float m_targetDir;
    float m_enemyDir;
    float m_enemySpeed;
};
