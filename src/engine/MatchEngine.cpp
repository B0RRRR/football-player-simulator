#include "MatchEngine.h"
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
    
    m_userSubbedOff = false;
    m_userStartReason = "";
    
    bool userIsPlaying = true;
    if (m_player) {
        int clubStr = m_playerClub->strength;
        int playerStr = (m_player->shooting + m_player->passing + m_player->tackling + m_player->goalkeeping) / 4;
        playerStr += (m_player->morale - 50) / 5; // Morale modifier

        if (m_player->injuredDays > 0) {
            userIsPlaying = false; m_userStartReason = "Status: INJURED";
        } else if (m_player->suspensionMatches > 0) {
            userIsPlaying = false; m_userStartReason = "Status: SUSPENDED";
        } else if (m_player->coachTrust < 30.0f) {
            userIsPlaying = false; m_userStartReason = "Status: BENCHED (Low Trust)";
        } else if (m_player->energy < 50.0f) {
            userIsPlaying = false; m_userStartReason = "Status: LEFT OUT (Too Tired)";
        } else if (playerStr < clubStr - 15) {
            userIsPlaying = false; m_userStartReason = "Status: BENCHED (Stats too low)";
        }
    } else {
        userIsPlaying = false;
    }
    
    m_userSubbedOff = !userIsPlaying;
    
    if (userIsPlaying) {
        if (rand() % 100 < 8) m_userInjuryMinute = 10 + rand() % 75;
        if (rand() % 100 < 5) m_userRedCardMinute = 10 + rand() % 75;
    }
    
    if (rand() % 100 < 10) {
        m_aiRedCardMinute = 10 + rand() % 75;
        m_aiRedCardIsHome = (rand() % 2 == 0);
        m_aiRedCardIndex = 1 + rand() % 10; // 1 to 10 to avoid Goalkeeper
    }
}

void MatchEngine::addLog(const std::string& msg, EventType type, bool isHome, EventOutcome outcome) {
    MatchEvent e;
    e.text = "[" + std::to_string(m_minute) + "'] " + msg;
    e.type = type;
    e.isHome = isHome;
    e.outcome = outcome;
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
        if (event.outcome == EventOutcome::RedCard) {
            if (event.isHome) m_homeStats.redCards++;
            else m_awayStats.redCards++;

            // Actually send a player off the pitch. hasRedCard() (which decides who is
            // drawn) reads the index lists, and only the scheduled/user red cards recorded
            // an index before - a red from simulateAIEvent or a minigame left all 11 on
            // the field. Reconcile: if the index list is short of the stat, remove a
            // random outfield player who isn't already off.
            std::vector<int>& idxList = event.isHome ? m_homeRedCards : m_awayRedCards;
            int stat = event.isHome ? m_homeStats.redCards : m_awayStats.redCards;
            if ((int)idxList.size() < stat) {
                for (int tries = 0; tries < 20; ++tries) {
                    int cand = 1 + rand() % 10; // 1..10, keep the keeper on
                    bool taken = false;
                    for (int r : idxList) if (r == cand) taken = true;
                    if (!taken) { idxList.push_back(cand); break; }
                }
            }
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
    
    if (m_minute == m_userInjuryMinute && !m_userSubbedOff) {
        addLog("INJURY! [USER]", EventType::Normal, m_isHome);
        m_userSubbedOff = true;
        m_userStartReason = "Status: INJURED IN MATCH";
        m_player->injuredDays = 14 + rand() % 21; // 2 to 5 weeks
    }
    
    if (m_minute == m_userRedCardMinute && !m_userSubbedOff && m_homeRedCards.empty() && m_awayRedCards.empty()) {
        addLog("RED CARD! [USER]", EventType::Card, m_isHome, EventOutcome::RedCard);
        m_userSubbedOff = true;
        m_userStartReason = "Status: SENT OFF (Red Card)";
        m_player->suspensionMatches = 2; // user suspended
        // Record the sent-off player (-1 = the user). The redCards STAT is incremented
        // once, in commitEvent, so it isn't double-counted.
        if (m_isHome) m_homeRedCards.push_back(-1);
        else m_awayRedCards.push_back(-1);
    }

    if (m_minute == m_aiRedCardMinute && m_homeRedCards.empty() && m_awayRedCards.empty()) {
        addLog("RED CARD! [AI] " + std::to_string(m_aiRedCardIndex), EventType::Card, m_aiRedCardIsHome, EventOutcome::RedCard);
        if (m_aiRedCardIsHome) m_homeRedCards.push_back(m_aiRedCardIndex);
        else m_awayRedCards.push_back(m_aiRedCardIndex);
    }
    
    int randVal = rand() % 100;
    
    int pStrength = m_playerClub->strength - m_homeStats.redCards * 15;
    if (!m_isHome) pStrength = m_playerClub->strength - m_awayStats.redCards * 15;
    
    int oStrength = m_opponentClub->strength - m_awayStats.redCards * 15;
    if (!m_isHome) oStrength = m_opponentClub->strength - m_homeStats.redCards * 15;
    
    int playerTeamChance = 12 + (pStrength - oStrength) / 6;
    int oppTeamChance = 12 + (oStrength - pStrength) / 6;
    
    if (playerTeamChance < 2) playerTeamChance = 2;
    if (oppTeamChance < 2) oppTeamChance = 2;
    
    float baseMomentum = (m_isHome ? (pStrength - oStrength) : (oStrength - pStrength)); 
    float currentMomentum = baseMomentum + ((rand() % 40) - 20);
    
    if (randVal < playerTeamChance) {
        currentMomentum += (m_isHome ? 30.0f : -30.0f);
        m_playerTeamAttacking = true;
        
        // When our team attacks and the user plays an attacking role, he always gets to
        // play it out - the ball is his. Previously this was a 40%/30% roll, so the user
        // could watch his side attack all match and never touch the ball.
        bool triggerMg = (m_player->position == PlayerPosition::Forward
                       || m_player->position == PlayerPosition::Midfielder);

        if (triggerMg && !m_userSubbedOff) {
            addLog("Minigame triggering for player", EventType::PendingMinigame, m_isHome);
        } else {
            simulateAIEvent(true);
        }
    } else if (randVal < playerTeamChance + oppTeamChance) {
        currentMomentum += (m_isHome ? -30.0f : 30.0f);
        m_playerTeamAttacking = false;
        
        // Same on the other side: when the opponent attacks and the user plays a
        // defending role, he always gets to try to stop it, so a defender/keeper/
        // midfielder is never a spectator to the whole match either.
        bool triggerMg = (m_player->position == PlayerPosition::Defender
                       || m_player->position == PlayerPosition::Goalkeeper
                       || m_player->position == PlayerPosition::Midfielder);

        if (triggerMg && !m_userSubbedOff) {
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
    
    // Dynamically calculate possession based on average momentum
    float avgMomentum = 0.0f;
    for (float m : m_momentumHistory) avgMomentum += m;
    avgMomentum /= m_momentumHistory.size();
    
    // avgMomentum is between -100 (Away dominance) and 100 (Home dominance)
    // Map -100..100 to 0..100 for home possession
    int homePossession = 50 + static_cast<int>(avgMomentum / 2.0f);
    if (homePossession > 80) homePossession = 80; // realistic cap
    if (homePossession < 20) homePossession = 20; // realistic floor
    
    m_homeStats.possession = homePossession;
    m_awayStats.possession = 100 - homePossession;
}

void MatchEngine::simulateAIEvent(bool playerTeamAttacking) {
    if (rand() % 100 < 15) {
        if (playerTeamAttacking) {
            addLog("GOAL! " + m_playerClub->name + " scores a brilliant team goal!", EventType::Goal, m_isHome, EventOutcome::Goal);
        } else {
            addLog("GOAL! " + m_opponentClub->name + " finds the back of the net!", EventType::Goal, !m_isHome, EventOutcome::Goal);
        }
    } else {
        if (rand() % 2 == 0) {
            addLog("Miss! The shot goes wide of the post.", EventType::Chance, playerTeamAttacking ? m_isHome : !m_isHome, EventOutcome::Miss);
        } else {
            addLog("Saved! The goalkeeper makes an easy stop.", EventType::Chance, playerTeamAttacking ? m_isHome : !m_isHome, EventOutcome::Saved);
        }
    }

    if (rand() % 100 < 15) {
        if (rand() % 100 < 10) {
            if (rand() % 2 == 0) {
                addLog("RED CARD! A horrible challenge leaves " + m_playerClub->name + " with 10 men!", EventType::Card, m_isHome, EventOutcome::RedCard);
            } else {
                addLog("RED CARD! A horrible challenge leaves " + m_opponentClub->name + " with 10 men!", EventType::Card, !m_isHome, EventOutcome::RedCard);
            }
        } else {
            if (rand() % 2 == 0) {
                addLog("Yellow card for a reckless tackle by " + m_playerClub->name + ".", EventType::Card, m_isHome, EventOutcome::YellowCard);
            } else {
                addLog("Yellow card for a reckless tackle by " + m_opponentClub->name + ".", EventType::Card, !m_isHome, EventOutcome::YellowCard);
            }
        }
    }
}

void MatchEngine::processMinigameResult(const MinigameResult& result) {
    float ratingDelta = result.success ? (0.3f + result.accuracy * 0.4f) : -(0.2f + result.accuracy * 0.4f);
    m_playerRating += ratingDelta;
    if (m_playerRating > 10.0f) m_playerRating = 10.0f;
    if (m_playerRating < 1.0f) m_playerRating = 1.0f;

    // Lost the ball without an attempt - dithered until robbed, or dispossessed by a
    // closing opponent. This is a turnover, not a missed shot, so it gets its own line
    // (the engine used to only know "shot missed", which is why idling read as
    // "missed a golden opportunity"). Possession passes to the opponent.
    if (result.variant == ActionVariant::Dispossessed) {
        // EventType::Normal, not Chance: it's a plain turnover. A Chance would make the
        // visual layer stage a full opponent attack + shot off the back of it, overstating
        // a simple loss of possession.
        addLog(m_player->name + " is robbed of possession!", EventType::Normal, !m_isHome, EventOutcome::TackleLost);
        m_state = MatchState::Simulating;
        return;
    }

    bool finesse = (result.variant == ActionVariant::Finesse);
    bool slide = (result.variant == ActionVariant::Slide);
    bool dive = (result.variant == ActionVariant::Dive);
    bool lofted = (result.variant == ActionVariant::Lofted);

    // Branch on WHAT the player did, not on what position he nominally plays. A defender
    // who wins the ball back can now carry it and shoot, so "position == Defender" no
    // longer implies "this was a tackle". MinigameResult::kind is the authority - that is
    // exactly what the typed contract exists for.
    if (result.success) {
        if (result.kind == MinigameActionKind::Shot) {
            if (finesse) addLog("GOAL!!! " + m_player->name + " curls a delicate finesse shot into the corner!", EventType::Goal, m_isHome, EventOutcome::Goal);
            else addLog("GOAL!!! " + m_player->name + " smashes a thunderous strike into the net!", EventType::Goal, m_isHome, EventOutcome::Goal);
            m_player->goals++;
            m_userGoalsScored++;
        } else if (result.kind == MinigameActionKind::Pass) {
            if (lofted) addLog("Inch-perfect through ball! " + m_player->name + " splits the defense and creates a huge chance!", EventType::Chance, m_isHome, EventOutcome::PassGood);
            else addLog("Great pass! " + m_player->name + " creates a dangerous chance.", EventType::Chance, m_isHome, EventOutcome::PassGood);
            simulateAIEvent(true);
        } else if (result.kind == MinigameActionKind::Tackle) {
            if (slide) addLog("Perfectly timed slide tackle! " + m_player->name + " dispossesses the attacker.", EventType::Chance, m_isHome, EventOutcome::TackleWon);
            else addLog("Great tackle! " + m_player->name + " wins the ball back.", EventType::Chance, m_isHome, EventOutcome::TackleWon);
        } else if (result.kind == MinigameActionKind::Save) {
            if (dive) addLog("Great save! " + m_player->name + " dives brilliantly to keep it out!", EventType::Chance, !m_isHome, EventOutcome::Saved);
            else addLog("What a save! " + m_player->name + " keeps the ball out!", EventType::Chance, !m_isHome, EventOutcome::Saved);
        }
    } else {
        if (result.kind == MinigameActionKind::Shot) {
            if (finesse) addLog(m_player->name + "'s finesse effort lacks conviction, straight at the keeper!", EventType::Chance, m_isHome, EventOutcome::Miss);
            else addLog(m_player->name + " misses a golden opportunity!", EventType::Chance, m_isHome, EventOutcome::Miss);
        } else if (result.kind == MinigameActionKind::Pass) {
            if (lofted) addLog(m_player->name + " tries an ambitious through ball but it's cut out by the defense.", EventType::Chance, !m_isHome, EventOutcome::PassBad);
            else addLog(m_player->name + " loses the ball with a bad pass.", EventType::Chance, !m_isHome, EventOutcome::PassBad);
        } else if (result.kind == MinigameActionKind::Tackle) {
            int foulRoll = rand() % 100;
            if (slide && foulRoll < 30) {
                addLog("Yellow card for a mistimed sliding tackle by " + m_player->name + "!", EventType::Card, m_isHome, EventOutcome::YellowCard);
            } else if (rand() % 100 < 50) {
                addLog("GOAL! " + m_opponentClub->name + " scores after a mistake by " + m_player->name + "!", EventType::Goal, !m_isHome, EventOutcome::Goal);
            } else {
                addLog(m_player->name + " gets beaten, but the opponent misses.", EventType::Chance, !m_isHome, EventOutcome::TackleLost);
            }
        } else if (result.kind == MinigameActionKind::Save) {
            if (dive) addLog("GOAL! " + m_opponentClub->name + " scores! " + m_player->name + " dives the wrong way!", EventType::Goal, !m_isHome, EventOutcome::Goal);
            else addLog("GOAL! " + m_opponentClub->name + " scores! " + m_player->name + " couldn't stop it.", EventType::Goal, !m_isHome, EventOutcome::Goal);
        }
    }

    m_state = MatchState::Simulating;
}
