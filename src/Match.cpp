#include "Match.h"
#include "Settings.h"
#include <cstdlib>

Match::Match(Player* player) : m_player(player), m_state(MatchState::Playing), m_timeAccumulator(0), m_minute(0), m_scoreUs(0), m_scoreThem(0) {
    addLog("Match started!");
}

void Match::addLog(const std::string& msg) {
    m_logs.push_back("[" + std::to_string(m_minute) + "'] " + msg);
    // Keep only last 5 logs for UI simplicity
    if (m_logs.size() > 5) {
        m_logs.erase(m_logs.begin());
    }
}

void Match::update(float deltaTime) {
    if (m_state != MatchState::Playing) return;

    m_timeAccumulator += deltaTime;
    // 1 real second = 5 game minutes
    if (m_timeAccumulator >= 1.0f) {
        m_timeAccumulator -= 1.0f;
        m_minute += 5;
        
        if (m_minute >= 90) {
            m_minute = 90;
            m_state = MatchState::Finished;
            addLog("Referee blows the final whistle!");
            return;
        }

        // Random events
        int r = rand() % 100;
        int opponentChance = 10;
        if (g_settings.difficulty == 2) opponentChance = 20; // Harder
        
        if (r < opponentChance) {
            m_scoreThem++;
            addLog("Opponent scores a goal!");
        } else if (r >= opponentChance && r < opponentChance + 20) {
            // Trigger key moment for the player
            m_state = MatchState::KeyMoment;
            addLog("You receive the ball near the box! What do you do?");
        } else {
            addLog("Ball is in the midfield...");
        }
    }
}

void Match::playerShoot() {
    int roll = rand() % 100;
    // Difficulty modifier
    int modifier = 0;
    if (g_settings.difficulty == 0) modifier = 20; // Easy
    if (g_settings.difficulty == 2) modifier = -20; // Hard

    if (roll < m_player->shooting + modifier) {
        m_scoreUs++;
        m_player->goals++;
        addLog("GOAL!!! What a fantastic strike by " + m_player->name + "!");
    } else {
        addLog("Miss! The shot goes wide.");
    }
    m_state = MatchState::Playing;
}

void Match::playerPass() {
    int roll = rand() % 100;
    int modifier = 0;
    if (g_settings.difficulty == 0) modifier = 20; // Easy
    if (g_settings.difficulty == 2) modifier = -20; // Hard

    if (roll < m_player->passing + modifier) {
        // Successful pass leads to a high chance of a goal by teammate
        if (rand() % 100 < 50) {
            m_scoreUs++;
            m_player->assists++;
            addLog("Great pass! Teammate scores! Assist for " + m_player->name + "!");
        } else {
            addLog("Good pass, but teammate misses the shot.");
        }
    } else {
        addLog("Bad pass, ball intercepted by defender.");
    }
    m_state = MatchState::Playing;
}
