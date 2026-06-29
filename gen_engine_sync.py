import os

cpp_code = """#include "MatchEngine.h"
#include "Player.h"
#include <cstdlib>
#include <string>

MatchEngine::MatchEngine(Club* playerClub, Club* opponentClub, bool isHome, Player* player)
    : m_playerClub(playerClub), m_opponentClub(opponentClub), m_player(player), m_isHome(isHome),
      m_minute(0), m_playerRating(6.0f), m_state(MatchState::Simulating) {
      
    std::string homeName = isHome ? playerClub->name : opponentClub->name;
    std::string awayName = isHome ? opponentClub->name : playerClub->name;
    
    // We can commit Kick-Off immediately
    MatchEvent startEvent;
    startEvent.text = "KICK-OFF! Match starts between " + homeName + " and " + awayName + ".";
    startEvent.type = EventType::Normal;
    startEvent.isHome = true;
    m_logs.push(startEvent);
    
    // Initial momentum is 0
    m_momentumHistory.push_back(0.0f);
}

void MatchEngine::addLog(const std::string& msg, EventType type, bool isHome) {
    MatchEvent e;
    e.text = "[" + std::to_string(m_minute) + "'] " + msg;
    e.type = type;
    e.isHome = isHome;
    m_logs.push(e);
}

MatchEvent MatchEngine::popRecentLog() {
    if (m_logs.empty()) return MatchEvent{"", EventType::Normal, true};
    MatchEvent log = m_logs.front();
    m_logs.pop();
    return log;
}

void MatchEngine::commitEvent(const MatchEvent& event) {
    // This is called by MatchScreen when the animation is completely finished.
    // We update stats based on event type.
    if (event.type == EventType::Goal) {
        if (event.isHome) { m_homeStats.goals++; m_homeStats.shots++; }
        else { m_awayStats.goals++; m_awayStats.shots++; }
    } else if (event.type == EventType::Chance) {
        if (event.isHome) m_homeStats.shots++;
        else m_awayStats.shots++;
    } else if (event.type == EventType::Card) {
        if (event.text.find("RED") != std::string::npos) {
            if (event.isHome) m_homeStats.redCards++;
            else m_awayStats.redCards++;
        } else {
            if (event.isHome) m_homeStats.yellowCards++;
            else m_awayStats.yellowCards++;
        }
    }
}

void MatchEngine::triggerMinigame() {
    m_state = MatchState::MinigameTriggered;
}

void MatchEngine::updateMinute() {
    if (m_state != MatchState::Simulating) return;
    
    m_minute++;
    if (m_minute >= 90) {
        addLog("FULL TIME! The referee blows the final whistle.", EventType::Normal, true);
        m_state = MatchState::Finished;
        return;
    }
    
    int randVal = rand() % 100;
    
    int pStrength = m_playerClub->strength - m_homeStats.redCards * 15;
    if (!m_isHome) pStrength = m_playerClub->strength - m_awayStats.redCards * 15;
    
    int oStrength = m_opponentClub->strength - m_awayStats.redCards * 15;
    if (!m_isHome) oStrength = m_opponentClub->strength - m_homeStats.redCards * 15;
    
    int playerTeamChance = 4 + (pStrength - oStrength) / 10;
    int oppTeamChance = 4 + (oStrength - pStrength) / 10;
    
    if (playerTeamChance < 1) playerTeamChance = 1;
    if (oppTeamChance < 1) oppTeamChance = 1;
    
    float baseMomentum = (m_isHome ? (pStrength - oStrength) : (oStrength - pStrength)); 
    float currentMomentum = baseMomentum + ((rand() % 40) - 20);
    
    if (randVal < playerTeamChance) {
        currentMomentum += (m_isHome ? 30.0f : -30.0f);
        m_playerTeamAttacking = true;
        
        bool triggerMg = false;
        if (m_player->position == PlayerPosition::Forward && (rand() % 100 < 40)) triggerMg = true;
        if (m_player->position == PlayerPosition::Midfielder && (rand() % 100 < 30)) triggerMg = true;
        
        if (triggerMg) {
            addLog("Minigame triggering for player", EventType::PendingMinigame, m_isHome);
        } else {
            simulateAIEvent(true);
        }
    } else if (randVal < playerTeamChance + oppTeamChance) {
        currentMomentum += (m_isHome ? -30.0f : 30.0f);
        m_playerTeamAttacking = false;
        
        bool triggerMg = false;
        if (m_player->position == PlayerPosition::Defender && (rand() % 100 < 40)) triggerMg = true;
        if (m_player->position == PlayerPosition::Goalkeeper && (rand() % 100 < 40)) triggerMg = true;
        if (m_player->position == PlayerPosition::Midfielder && (rand() % 100 < 20)) triggerMg = true;
        
        if (triggerMg) {
            addLog("Minigame triggering against player", EventType::PendingMinigame, !m_isHome);
        } else {
            simulateAIEvent(false);
        }
    }
    
    if (currentMomentum > 100.0f) currentMomentum = 100.0f;
    if (currentMomentum < -100.0f) currentMomentum = -100.0f;
    
    if (!m_momentumHistory.empty()) {
        currentMomentum = (m_momentumHistory.back() * 0.6f) + (currentMomentum * 0.4f);
    }
    m_momentumHistory.push_back(currentMomentum);
}

void MatchEngine::simulateAIEvent(bool playerTeamAttacking) {
    if (rand() % 100 < 15) {
        if (playerTeamAttacking) {
            addLog("GOAL! " + m_playerClub->name + " scores a brilliant team goal!", EventType::Goal, m_isHome);
        } else {
            addLog("GOAL! " + m_opponentClub->name + " finds the back of the net!", EventType::Goal, !m_isHome);
        }
    } else {
        if (rand() % 2 == 0) {
            addLog("Miss! The shot goes wide of the post.", EventType::Chance, playerTeamAttacking ? m_isHome : !m_isHome);
        } else {
            addLog("Saved! The goalkeeper makes an easy stop.", EventType::Chance, playerTeamAttacking ? m_isHome : !m_isHome);
        }
    }
    
    if (rand() % 100 < 15) {
        if (rand() % 100 < 10) {
            if (rand() % 2 == 0) {
                addLog("RED CARD! A horrible challenge leaves " + m_playerClub->name + " with 10 men!", EventType::Card, m_isHome);
            } else {
                addLog("RED CARD! A horrible challenge leaves " + m_opponentClub->name + " with 10 men!", EventType::Card, !m_isHome);
            }
        } else {
            if (rand() % 2 == 0) {
                addLog("Yellow card for a reckless tackle by " + m_playerClub->name + ".", EventType::Card, m_isHome);
            } else {
                addLog("Yellow card for a reckless tackle by " + m_opponentClub->name + ".", EventType::Card, !m_isHome);
            }
        }
    }
}

void MatchEngine::processMinigameResult(bool success) {
    if (success) {
        m_playerRating += 0.5f;
        if (m_playerRating > 10.0f) m_playerRating = 10.0f;
        
        if (m_player->position == PlayerPosition::Forward) {
            addLog("GOAL!!! " + m_player->name + " scores a magnificent goal!", EventType::Goal, m_isHome);
            m_player->goals++;
        } else if (m_player->position == PlayerPosition::Midfielder) {
            if (m_playerTeamAttacking) {
                if (rand() % 2 == 0) {
                    addLog("GOAL! " + m_player->name + " provides a beautiful assist!", EventType::Goal, m_isHome);
                    m_player->assists++;
                } else {
                    addLog("Great pass! " + m_player->name + " creates a dangerous chance.", EventType::Chance, m_isHome);
                }
            } else {
                addLog("Great interception! " + m_player->name + " wins the ball back.", EventType::Chance, m_isHome);
            }
        } else if (m_player->position == PlayerPosition::Defender) {
            addLog("Great tackle! " + m_player->name + " stops a dangerous attack.", EventType::Chance, m_isHome);
        } else if (m_player->position == PlayerPosition::Goalkeeper) {
            addLog("What a save! " + m_player->name + " keeps the ball out!", EventType::Chance, m_isHome);
        }
    } else {
        m_playerRating -= 0.3f;
        if (m_playerRating < 1.0f) m_playerRating = 1.0f;
        
        if (m_player->position == PlayerPosition::Forward) {
            addLog(m_player->name + " misses a golden opportunity!", EventType::Chance, m_isHome);
        } else if (m_player->position == PlayerPosition::Midfielder) {
            addLog(m_player->name + " loses the ball with a bad pass.", EventType::Chance, m_isHome);
        } else if (m_player->position == PlayerPosition::Defender) {
            if (rand() % 100 < 50) {
                addLog("GOAL! " + m_opponentClub->name + " scores after a mistake by " + m_player->name + "!", EventType::Goal, !m_isHome);
            } else {
                addLog(m_player->name + " gets beaten, but the opponent misses.", EventType::Chance, !m_isHome);
            }
        } else if (m_player->position == PlayerPosition::Goalkeeper) {
            addLog("GOAL! " + m_opponentClub->name + " scores! " + m_player->name + " couldn't stop it.", EventType::Goal, !m_isHome);
        }
    }
    
    m_state = MatchState::Simulating;
}
"""

with open("src/MatchEngine.cpp", "w") as f:
    f.write(cpp_code)
print("MatchEngine.cpp updated.")
