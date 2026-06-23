#pragma once
#include <string>
#include <vector>

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

class Database {
public:
    Database();
    
    // Initialize the hardcoded database
    void init();
    
    // Reset league table stats for all clubs
    void resetStats();
    
    const std::vector<League>& getLeagues() const { return m_leagues; }
    const League* getLeague(const std::string& name) const;
    Club* getClub(const std::string& leagueName, const std::string& clubName);

private:
    std::vector<League> m_leagues;
};
