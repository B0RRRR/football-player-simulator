#pragma once
#include <string>
#include <vector>
#include <map>

struct Club {
    std::string name;
    int strength; // 0 to 100
    
    // League Stats
    int points = 0;
    int wins = 0;
    int draws = 0;
    int losses = 0;
    int goalsFor = 0;
    int goalsAgainst = 0;
};

struct League {
    std::string name;
    std::vector<Club> clubs;
};

struct CupMatch {
    Club* home = nullptr;
    Club* away = nullptr;
    int homeGoalsLeg1 = 0;
    int awayGoalsLeg1 = 0;
    int homeGoalsLeg2 = 0;
    int awayGoalsLeg2 = 0;
    int homePenalties = 0;
    int awayPenalties = 0;
    bool leg1Played = false;
    bool leg2Played = false;
    bool isFinal = false;
    Club* winner = nullptr;
};

struct CupRound {
    std::string name;
    std::vector<CupMatch> matches;
    bool isCompleted = false;
};

struct Tournament {
    std::string name;
    std::vector<CupRound> rounds;
    int currentRoundIndex = 0;
    bool isFinished = false;
    Club* winner = nullptr;
};

class Database {
    friend class SaveManager;
public:
    Database();
    
    // Initialize the hardcoded database
    void init();
    
    // Reset league table stats for all clubs
    void resetStats();
    
    // Archive current league tables into history
    void archiveCurrentSeason(int year);
    
    // Archive tournaments into history
    void archiveClubTournaments(int year);
    void archiveInternationalTournaments(int year);
    
    const std::vector<League>& getLeagues() const { return m_leagues; }
    const std::map<int, std::vector<League>>& getLeagueHistory() const { return m_leagueHistory; }
    const League* getLeague(const std::string& name) const;
    const League* getNationalTeams() const { return &m_nationalTeams; }
    Club* getClub(const std::string& leagueName, const std::string& clubName);
    
    void processRelegation(int year);
    
    void initTournaments(const std::vector<Club*>& clClubs, const std::vector<Club*>& elClubs);
    void advanceTournamentRound(Tournament& t);
    
    Tournament& getChampionsLeague() { return m_championsLeague; }
    Tournament& getEuropaLeague() { return m_europaLeague; }
    
    Tournament& getWorldCup() { return m_worldCup; }
    Tournament& getEuroCup() { return m_euroCup; }
    
    const std::map<int, Tournament>& getChampionsLeagueHistory() const { return m_championsLeagueHistory; }
    const std::map<int, Tournament>& getEuropaLeagueHistory() const { return m_europaLeagueHistory; }
    const std::map<int, Tournament>& getWorldCupHistory() const { return m_worldCupHistory; }
    const std::map<int, Tournament>& getEuroCupHistory() const { return m_euroCupHistory; }
    
    void initNationalTeams();
    void generateWorldCup();
    void generateEuroCup();

private:
    std::vector<League> m_leagues;
    std::map<int, std::vector<League>> m_leagueHistory;
    League m_nationalTeams; // Holds all national teams
    Tournament m_championsLeague;
    Tournament m_europaLeague;
    Tournament m_worldCup;
    Tournament m_euroCup;
    
    std::map<int, Tournament> m_championsLeagueHistory;
    std::map<int, Tournament> m_europaLeagueHistory;
    std::map<int, Tournament> m_worldCupHistory;
    std::map<int, Tournament> m_euroCupHistory;
};
