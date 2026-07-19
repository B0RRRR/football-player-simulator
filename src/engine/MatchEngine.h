#pragma once
#include <string>
#include <vector>
#include <queue>
#include "Database.h"

struct MatchStats {
    int goals = 0;
    int shots = 0;
    int possession = 50;
    int yellowCards = 0;
    int redCards = 0;

};

enum class MatchState {
    Simulating,
    MinigameTriggered,
    Finished
};

enum class EventType {
    Normal,
    Goal,
    Chance,
    Foul,
    Card,
    PendingMinigame
};

enum class EventOutcome {
    None,
    Goal,
    Miss,
    Saved,
    Blocked,
    TackleWon,
    TackleLost,
    Intercepted,
    PassGood,
    PassBad,
    Foul,
    YellowCard,
    RedCard
};

struct MatchEvent {
    std::string text;
    EventType type = EventType::Normal;
    bool isHome = true; // true if it's the home team doing the action
    EventOutcome outcome = EventOutcome::None;
};

enum class MinigameActionKind {
    Shot,
    Pass,
    Tackle,
    Save,
    Dribble
};

enum class ActionVariant {
    Default,
    Power,
    Finesse,
    Lofted,
    Slide,
    Dive,
    Dispossessed // ball lost without an attempt (idle / robbed) - a turnover, not a miss
};

struct MinigameResult {
    bool success;
    MinigameActionKind kind;
    float power = 0.5f;    // 0..1, normalized shot/pass power
    float accuracy = 0.5f; // 0..1, normalized placement accuracy
    ActionVariant variant = ActionVariant::Default;
};

class Player; // Forward declaration

class MatchEngine {
public:
    MatchEngine(Club* playerClub, Club* opponentClub, bool isHome, Player* player);
    
    void updateMinute();
    void commitEvent(const MatchEvent& event);
    void triggerMinigame();
    void processMinigameResult(const MinigameResult& result);
    
    void setPlayerTeamAttacking(bool att) { m_playerTeamAttacking = att; }
    
    MatchState getState() const { return m_state; }
    int getMinute() const { return m_minute; }
    const MatchStats& getPlayerTeamStats() const { return m_isHome ? m_homeStats : m_awayStats; }
    const MatchStats& getOpponentTeamStats() const { return m_isHome ? m_awayStats : m_homeStats; }
    float getPlayerRating() const { return m_playerRating; }
    
    Club* getPlayerClub() const { return m_playerClub; }
    Club* getOpponentClub() const { return m_opponentClub; }
    
    MatchEvent popRecentLog();
    bool hasLogs() const { return !m_logs.empty(); }
    
    int getUserGoalsScored() const { return m_userGoalsScored; }

    const std::vector<float>& getMomentumHistory() const { return m_momentumHistory; }

    int getHomeScore() const { return m_homeStats.goals; }
    int getAwayScore() const { return m_awayStats.goals; }
    
    bool isHome() const { return m_isHome; }
    
    const std::vector<int>& getHomeRedCards() const { return m_homeRedCards; }
    const std::vector<int>& getAwayRedCards() const { return m_awayRedCards; }
    bool isUserSubbedOff() const { return m_userSubbedOff; }
    std::string getUserStartReason() const { return m_userStartReason; }

private:
    void simulateAIEvent(bool playerTeamAttacking);
    void addLog(const std::string& msg, EventType type = EventType::Normal, bool isHome = true, EventOutcome outcome = EventOutcome::None);

    Club* m_playerClub;
    Club* m_opponentClub;
    Player* m_player;
    bool m_isHome;
    bool m_playerTeamAttacking;

    
    MatchStats m_homeStats;
    MatchStats m_awayStats;
    
    int m_minute;
    float m_playerRating;
    MatchState m_state;
    
    std::vector<int> m_homeRedCards;
    std::vector<int> m_awayRedCards;
    bool m_userSubbedOff;
    std::string m_userStartReason;
    
    int m_userInjuryMinute = -1;
    int m_userRedCardMinute = -1;
    int m_aiRedCardMinute = -1;
    bool m_aiRedCardIsHome = false;
    int m_aiRedCardIndex = -1;
    
    int m_userGoalsScored = 0;
    
    std::queue<MatchEvent> m_logs;
    std::vector<float> m_momentumHistory;
};
