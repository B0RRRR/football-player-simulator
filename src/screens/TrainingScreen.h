#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>

class DrillArena;

enum class TrainingState {
    Intro,
    Playing,
    Result
};

class TrainingScreen : public Screen {
public:
    TrainingScreen();
    ~TrainingScreen() override;           // defined in .cpp where DrillArena is complete

    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;
    bool playsClickOnPress() const override { return false; } // drill drives its own audio
    // Training fades the menu music out and runs the stadium ambience (intershum) - but there are
    // no fans at a training session, so crowd reactions (celebration/whistle) never fire here: the
    // drill simply never calls AudioManager::reaction(), unlike a real match.
    bool wantsMenuMusic() const override { return false; }
    bool wantsMatchAmbience() const override { return false; } // no crowd/intershum at training

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

    // New drill path (reuses the match minigames via DrillArena). Used for roles that have a
    // rebuilt drill; the old per-role code below is the fallback until they're all migrated.
    bool m_useDrill = false;
    std::unique_ptr<DrillArena> m_drill;
    sf::View m_pitchView;               // 1280x720, matching the match's coordinate frame
    int m_drillReps = 0, m_drillCount = 0, m_drillGoals = 0;
    float m_drillResultTimer = 0.f;
    
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
    float m_attackerDirX;
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
