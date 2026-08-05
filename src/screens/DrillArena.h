#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

// A tiny, self-contained pitch scenario that runs ONE match minigame in isolation, for
// training. It reuses ShotPath (flight geometry) and PitchRenderer (drawing) so a drill looks
// and plays exactly like the real thing, and future art plugs in through the same renderer.
// No MatchEngine: it just reports each attempt's success, and TrainingScreen tallies XP.
class DrillArena {
public:
    enum class Kind {
        ForwardFinish,    // ball at the box; draw a shot past the keeper
        DefenderDuel,     // an attacker jinks the ball at you; click it to nick it
        GoalkeeperSave,   // shots rain in; read the height and click to dive
        MidfielderPass    // team-mate opens up; draw a pass past a marker
    };

    enum class Result { Pending, Success, Fail };

    DrillArena(Kind kind, int primaryStat, int dribbling, int keeperStrength);

    void handleInput(sf::RenderWindow& window, const sf::Event& event, const sf::View& view);
    void update(float dt);
    void draw(sf::RenderWindow& window);

    bool repFinished() const { return m_result != Result::Pending; }
    Result result() const { return m_result; }
    void nextRep();
    std::string prompt() const;

private:
    void setupRep();
    void launchShot();
    void resolveShotAtGoal(float crossY);
    void resolveDuel(bool won);
    void resolveKeeper(float diveY);
    void launchPass();
    void resolvePass(bool completed);
    void tryDribble();                   // carrier drills: take on the nearest opponent
    static sf::Vector2f moveToward(sf::Vector2f from, sf::Vector2f to, float step);

    Kind m_kind;
    int  m_primaryStat;
    int  m_dribbling;
    int  m_keeperStrength;

    // Scene (world coords, same frame as the match).
    sf::Vector2f m_ball;
    sf::Vector2f m_user;                 // your dot
    sf::Vector2f m_keeper;
    sf::Vector2f m_attacker;
    std::vector<sf::Vector2f> m_mates;   // pass targets
    std::vector<sf::Vector2f> m_markers; // opponents in the lane
    std::vector<bool> m_beaten;          // markers already dribbled past
    bool m_carrying = false;             // gliding past a beaten opponent
    sf::Vector2f m_carryTarget;
    bool m_qPrev = false;                // edge-detect for the dribble key (polled, not evented)

    // Draw-a-shot/pass state.
    enum class Phase { Ready, Aiming, Power, Flight, Live, Done };
    Phase m_phase = Phase::Ready;
    std::vector<sf::Vector2f> m_path;
    float m_pathLen = 0.f;
    sf::Vector2f m_from;
    sf::Vector2f m_endDir{1.f, 0.f};
    float m_powerT = 0.f, m_powerDir = 1.f;
    float m_flightDist = 0.f, m_flightSpeed = 600.f;
    float m_shotTargetY = 290.f;

    // Duel / keeper timers and feints.
    float m_liveT = 0.f;                 // how long the live phase has run
    float m_feintClock = 0.f;
    sf::Vector2f m_feintVec, m_feintTarget;
    bool m_acted = false;                // one lunge / one dive
    int  m_passTargetIdx = -1;
    bool m_shotReleased = false;         // keeper drill: has the striker hit it yet
    float m_shotDelay = 0.f;             // keeper drill: build-up time before a shot is allowed
    int  m_carrierIdx = 0;               // keeper drill: which attacker (in m_markers) has the ball
    bool m_ballInPass = false;           // keeper drill: ball currently travelling between attackers
    int  m_passToIdx = -1;               // keeper drill: pass destination attacker

    Result m_result = Result::Pending;
    bool m_keeperDiving = false;
    sf::Vector2f m_keeperDiveTo;
    float m_resultTimer = 0.f;
};
