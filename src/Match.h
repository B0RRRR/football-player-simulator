#pragma once
#include "Player.h"
#include <string>
#include <vector>

enum class MatchState {
    Playing,
    KeyMoment, // Waiting for player input
    Finished
};

class Match {
public:
    Match(Player* player);

    void update(float deltaTime);
    
    // Player actions during key moment
    void playerShoot();
    void playerPass();

    int getMinute() const { return m_minute; }
    MatchState getState() const { return m_state; }
    const std::vector<std::string>& getLogs() const { return m_logs; }
    int getScoreUs() const { return m_scoreUs; }
    int getScoreThem() const { return m_scoreThem; }

private:
    void addLog(const std::string& msg);

    Player* m_player;
    MatchState m_state;
    
    float m_timeAccumulator;
    int m_minute;
    
    int m_scoreUs;
    int m_scoreThem;
    
    std::vector<std::string> m_logs;
};
