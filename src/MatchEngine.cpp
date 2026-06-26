#include "MatchEngine.h"
#include "Player.h"
#include <cstdlib>
#include <string>

MatchEngine::MatchEngine(Club* playerClub, Club* opponentClub, bool isHome, Player* player)
    : m_playerClub(playerClub), m_opponentClub(opponentClub), m_player(player), m_isHome(isHome),
      m_minute(0), m_playerRating(6.0f), m_state(MatchState::Simulating) {
      
    std::string homeName = isHome ? playerClub->name : opponentClub->name;
    std::string awayName = isHome ? opponentClub->name : playerClub->name;
    
    addLog("KICK-OFF! Match starts between " + homeName + " and " + awayName + ".");
}

void MatchEngine::addLog(const std::string& msg) {
    m_logs.push("[" + std::to_string(m_minute) + "'] " + msg);
}

std::string MatchEngine::popRecentLog() {
    if (m_logs.empty()) return "";
    std::string log = m_logs.front();
    m_logs.pop();
    return log;
}

void MatchEngine::updateMinute() {
    if (m_state != MatchState::Simulating) return;
    
    m_minute++;
    if (m_minute >= 90) {
        addLog("FULL TIME! The referee blows the final whistle.");
        m_state = MatchState::Finished;
        return;
    }
    
    // Determine random event
    int randVal = rand() % 100;
    
    // Base chances
    int pStrength = m_playerClub->strength - m_homeStats.redCards * 15; // Assuming player is home? Wait, I need to check m_isHome
    if (!m_isHome) pStrength = m_playerClub->strength - m_awayStats.redCards * 15;
    
    int oStrength = m_opponentClub->strength - m_awayStats.redCards * 15;
    if (!m_isHome) oStrength = m_opponentClub->strength - m_homeStats.redCards * 15;
    
    // Roughly 10-15 chances per match per team based on strength differences
    int playerTeamChance = 4 + (pStrength - oStrength) / 10;
    int oppTeamChance = 4 + (oStrength - pStrength) / 10;
    
    if (playerTeamChance < 1) playerTeamChance = 1;
    if (oppTeamChance < 1) oppTeamChance = 1;
    
    // In any given minute:
    // 0 to playerTeamChance -> Player's team gets a chance
    // playerTeamChance to playerTeamChance+oppTeamChance -> Opponent gets a chance
    if (randVal < playerTeamChance) {
        // Player's team attacking
        if (m_isHome) m_homeStats.shots++; else m_awayStats.shots++;
        
        // Minigame only for Forward or Midfielder when attacking
        bool triggerMinigame = false;
        if (m_player->position == PlayerPosition::Forward && (rand() % 100 < 40)) triggerMinigame = true;
        if (m_player->position == PlayerPosition::Midfielder && (rand() % 100 < 30)) triggerMinigame = true;
        
        if (triggerMinigame) {
            m_state = MatchState::MinigameTriggered;
            m_playerTeamAttacking = true;
        } else {
            simulateAIEvent(true); // Player's team AI simulates chance
        }
    } else if (randVal < playerTeamChance + oppTeamChance) {
        // Opponent's team attacking
        if (m_isHome) m_awayStats.shots++; else m_homeStats.shots++;
        
        // Minigame only for Defender, Goalkeeper, or Midfielder when defending
        bool triggerMinigame = false;
        if (m_player->position == PlayerPosition::Defender && (rand() % 100 < 40)) triggerMinigame = true;
        if (m_player->position == PlayerPosition::Goalkeeper && (rand() % 100 < 40)) triggerMinigame = true;
        if (m_player->position == PlayerPosition::Midfielder && (rand() % 100 < 20)) triggerMinigame = true;
        
        if (triggerMinigame) {
            m_state = MatchState::MinigameTriggered;
            m_playerTeamAttacking = false;
        } else {
            simulateAIEvent(false); // Opponent simulates chance
        }
    }
}

void MatchEngine::simulateAIEvent(bool playerTeamAttacking) {
    // If it's a goal (~15% of shots go in)
    if (rand() % 100 < 15) {
        if (playerTeamAttacking) {
            if (m_isHome) m_homeStats.goals++; else m_awayStats.goals++;
            addLog("GOAL! " + m_playerClub->name + " scores a brilliant team goal!");
        } else {
            if (m_isHome) m_awayStats.goals++; else m_homeStats.goals++;
            addLog("GOAL! " + m_opponentClub->name + " finds the back of the net!");
        }
    } else {
        if (rand() % 2 == 0) {
            addLog("Miss! The shot goes wide of the post.");
        } else {
            addLog("Saved! The goalkeeper makes an easy stop.");
        }
    }
    
    // Cards
    if (rand() % 100 < 15) { // 15% chance of a foul leading to a card (increased from 5%)
        if (rand() % 100 < 10) { // 10% of cards are RED
            if (rand() % 2 == 0) {
                if (m_isHome) m_homeStats.redCards++; else m_awayStats.redCards++;
                addLog("RED CARD! A horrible challenge leaves " + m_playerClub->name + " with 10 men!");
            } else {
                if (m_isHome) m_awayStats.redCards++; else m_homeStats.redCards++;
                addLog("RED CARD! A horrible challenge leaves " + m_opponentClub->name + " with 10 men!");
            }
        } else {
            if (rand() % 2 == 0) {
                if (m_isHome) m_homeStats.yellowCards++; else m_awayStats.yellowCards++;
                addLog("Yellow card for a reckless tackle by " + m_playerClub->name + ".");
            } else {
                if (m_isHome) m_awayStats.yellowCards++; else m_homeStats.yellowCards++;
                addLog("Yellow card for a reckless tackle by " + m_opponentClub->name + ".");
            }
        }
    }
}

void MatchEngine::processMinigameResult(bool success) {
    if (success) {
        m_playerRating += 0.5f;
        if (m_playerRating > 10.0f) m_playerRating = 10.0f;
        
        // If forward or midfielder, maybe it's a goal
        if (m_player->position == PlayerPosition::Forward) {
            if (m_isHome) m_homeStats.goals++; else m_awayStats.goals++;
            addLog("GOAL!!! " + m_player->name + " scores a magnificent goal!");
            m_player->goals++;
        } else if (m_player->position == PlayerPosition::Midfielder) {
            if (m_playerTeamAttacking) {
                if (rand() % 2 == 0) {
                    if (m_isHome) m_homeStats.goals++; else m_awayStats.goals++;
                    addLog("GOAL! " + m_player->name + " provides a beautiful assist!");
                    m_player->assists++;
                } else {
                    addLog("Great pass! " + m_player->name + " creates a dangerous chance.");
                }
            } else {
                addLog("Great interception! " + m_player->name + " wins the ball back.");
            }
        } else if (m_player->position == PlayerPosition::Defender) {
            addLog("Great tackle! " + m_player->name + " stops a dangerous attack.");
        } else if (m_player->position == PlayerPosition::Goalkeeper) {
            addLog("What a save! " + m_player->name + " keeps the ball out!");
        }
    } else {
        m_playerRating -= 0.3f;
        if (m_playerRating < 1.0f) m_playerRating = 1.0f;
        
        if (m_player->position == PlayerPosition::Forward) {
            addLog(m_player->name + " misses a golden opportunity!");
        } else if (m_player->position == PlayerPosition::Midfielder) {
            addLog(m_player->name + " loses the ball with a bad pass.");
        } else if (m_player->position == PlayerPosition::Defender) {
            // Opponent might score!
            if (rand() % 100 < 50) {
                if (m_isHome) m_awayStats.goals++; else m_homeStats.goals++;
                addLog("GOAL! " + m_player->name + " fails to tackle, and the opponent scores!");
            } else {
                addLog(m_player->name + " is beaten, but the shot misses.");
            }
        } else if (m_player->position == PlayerPosition::Goalkeeper) {
            if (m_isHome) m_awayStats.goals++; else m_homeStats.goals++;
            addLog("GOAL! " + m_player->name + " dives but can't reach it.");
        }
    }
    
    m_state = MatchState::Simulating;
}
