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
    GoalKick,
    PassingScript,
    PressingScript,
    Foul
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
    bool hasRedCard(int globalIdx) const;
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
    sf::RectangleShape m_btnSkipRect;
    sf::Text m_btnSkipText;
    
    int m_foulPlayerIdx;
    int m_foulVictimIdx;
    
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
    int m_attackType;
    int m_attackWingerIdx;
    float m_shotTargetY;
    int m_attackFwdIdx;
    int m_attackPhase;
    
    float m_simTimer;
    
    // Minigame Elements
    bool m_minigameActive;
    bool m_midfielderSoloRun;
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
    
    struct Button {
        sf::RectangleShape rect;
        sf::Text text;
        std::string action;
        sf::Color baseColor = sf::Color(100, 100, 100);
        bool isHovered = false;
    };
    std::vector<Button> m_speedButtons;
    int m_matchSpeedMode; // 0=Slow, 1=Normal, 2=Fast
    bool m_isMinigameResultPending;
    float m_scriptTimer;
};
