#include "Match.h"
#include "Player.h"
#include "Database.h"
#include "Settings.h"
#include <cstdlib>

Match::Match(Player* player) : m_player(player), m_state(MatchState::Playing), m_timeAccumulator(0), m_minute(0), m_scoreUs(0), m_scoreThem(0) {
    evaluateCoachDecision();
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
            m_player->experience += 50; // Match completion bonus
            addLog("Referee blows the final whistle! (+50 XP)");
            return;
        }

        // Energy drain, substitutions, and injuries
        if (m_playerStatus == PlayerMatchStatus::Starter) {
            // Drain energy roughly ~40 over 90 mins
            if (rand() % 100 < 45) {
                m_player->energy -= 1;
            }
            
            // Injury chance
            if (rand() % 1000 < 5) { // 0.5% chance per 5 minutes ~ 9% per game
                m_player->injuredDays = 7 + (rand() % 14); // 1 to 3 weeks
                m_playerStatus = PlayerMatchStatus::Out;
                addLog("CRITICAL INJURY! You were tackled hard and had to be carried off the pitch!");
            }
            else if (m_player->energy < 15 && m_minute > 60) {
                m_playerStatus = PlayerMatchStatus::Bench;
                addLog("Coach substitutes you out due to exhaustion.");
            }
        } else if (m_playerStatus == PlayerMatchStatus::Bench && m_minute > 55) {
            // Chance to be subbed in (higher if losing)
            int subChance = 25;
            if (m_scoreUs < m_scoreThem) subChance = 40;
            
            if (rand() % 100 < subChance) {
                m_playerStatus = PlayerMatchStatus::Starter;
                addLog("Coach subs you IN! Make an impact!");
            }
        }

        // Random events
        int r = rand() % 100;
        int opponentChance = 10;
        if (g_settings.difficulty == 2) opponentChance = 20; // Harder
        
        if (r < opponentChance) {
            m_scoreThem++;
            addLog("Opponent scores a goal!");
        } else if (r >= opponentChance && r < opponentChance + 20) {
            // Trigger key moment for the player ONLY if they are playing
            if (m_playerStatus == PlayerMatchStatus::Starter) {
                m_state = MatchState::KeyMoment;
                addLog("You receive the ball near the box! What do you do?");
            } else {
                addLog("Your team attacks, but the shot is saved.");
            }
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
        m_player->experience += 30;
        addLog("GOAL!!! What a fantastic strike by " + m_player->name + "! (+30 XP)");
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
            m_player->experience += 20;
            addLog("Great pass! Teammate scores! Assist for " + m_player->name + "! (+20 XP)");
        } else {
            addLog("Good pass, but teammate misses the shot.");
        }
    } else {
        addLog("Bad pass, ball intercepted by defender.");
    }
    m_state = MatchState::Playing;
}

void Match::evaluateCoachDecision() {
    if (!m_player->currentClub) {
        m_playerStatus = PlayerMatchStatus::Starter;
        return;
    }
    
    if (m_player->injuredDays > 0) {
        m_playerStatus = PlayerMatchStatus::Out;
        addLog("Coach decision: You are injured and cannot play.");
        return;
    }

    int clubStr = m_player->currentClub->strength;
    int playerStr = (m_player->shooting + m_player->passing + m_player->tackling + m_player->goalkeeping) / 4; 
    
    // Apply morale modifier (-10 to +10)
    int moraleModifier = (m_player->morale - 50) / 5;
    playerStr += moraleModifier;
    
    // Very tired -> Out
    if (m_player->energy < 20) {
        m_playerStatus = PlayerMatchStatus::Out;
        addLog("Coach decision: You are too tired and left out of the squad.");
    } 
    // Moderately tired or stats are way too low -> Bench
    else if (m_player->energy < 60 || playerStr < clubStr - 25) {
        m_playerStatus = PlayerMatchStatus::Bench;
        addLog("Coach decision: You start on the bench.");
    } 
    // Fit and decent stats -> Starter
    else {
        m_playerStatus = PlayerMatchStatus::Starter;
        addLog("Coach decision: You are in the starting XI!");
    }
}
