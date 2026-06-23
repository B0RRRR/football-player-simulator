#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

enum class TrainingState {
    Intro,
    Playing,
    Result
};

class TrainingScreen : public Screen {
public:
    TrainingScreen();
    
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    void initGame();
    void updateGame(float dt);
    void drawGame(sf::RenderWindow& window);
    void finishGame();

    TrainingState m_state;
    int m_score;
    int m_maxScore;
    int m_xpEarned;
    float m_timeRemaining;
    
    sf::Text m_mainText;
    sf::Text m_infoText;
    sf::RectangleShape m_btnRect;
    sf::Text m_btnText;

    // Mini-game specific variables
    // Forward (Shooting)
    sf::RectangleShape m_goalRect;
    sf::RectangleShape m_targetRect;
    sf::RectangleShape m_powerBarBg;
    sf::RectangleShape m_powerBarFill;
    float m_targetDir;
    float m_powerDir;
    float m_powerValue;
    int m_shotsTaken;

    // Midfielder (Passing)
    struct Teammate {
        sf::CircleShape shape;
        float timeAlive;
    };
    std::vector<Teammate> m_teammates;
    float m_spawnTimer;
    int m_passesAttempted;

    // Defender (Tackling)
    sf::CircleShape m_tackleZone;
    sf::RectangleShape m_attacker;
    float m_attackerSpeed;
    int m_tacklesAttempted;

    // Goalkeeper (Saving)
    struct Ball {
        sf::CircleShape shape;
        sf::Vector2f dir;
    };
    std::vector<Ball> m_balls;
    float m_ballSpawnTimer;
    int m_savesAttempted;
};
