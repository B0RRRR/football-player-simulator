#include "Database.h"

Database::Database() {
}

void Database::init() {
    // English Premier League
    League epl;
    epl.name = "Premier League";
    epl.clubs = {
        {"Manchester City", 90},
        {"Arsenal", 85},
        {"Liverpool", 85},
        {"Manchester United", 80},
        {"Chelsea", 80},
        {"Tottenham Hotspur", 78}
    };
    m_leagues.push_back(epl);

    // La Liga
    League laLiga;
    laLiga.name = "La Liga";
    laLiga.clubs = {
        {"Real Madrid", 92},
        {"Barcelona", 85},
        {"Atletico Madrid", 83},
        {"Sevilla", 75},
        {"Valencia", 73},
        {"Real Sociedad", 78}
    };
    m_leagues.push_back(laLiga);

    // Serie A
    League serieA;
    serieA.name = "Serie A";
    serieA.clubs = {
        {"Juventus", 83},
        {"Inter Milan", 86},
        {"AC Milan", 82},
        {"Napoli", 84},
        {"AS Roma", 80},
        {"Lazio", 78}
    };
    m_leagues.push_back(serieA);

    // Bundesliga
    League bundesliga;
    bundesliga.name = "Bundesliga";
    bundesliga.clubs = {
        {"Bayern Munich", 89},
        {"Borussia Dortmund", 82},
        {"Bayer Leverkusen", 84},
        {"RB Leipzig", 81},
        {"Eintracht Frankfurt", 76},
        {"Borussia M'gladbach", 74}
    };
    m_leagues.push_back(bundesliga);

    // Ligue 1
    League ligue1;
    ligue1.name = "Ligue 1";
    ligue1.clubs = {
        {"Paris SG", 88},
        {"Monaco", 79},
        {"Marseille", 78},
        {"Lyon", 75},
        {"Lille", 77},
        {"Rennes", 74}
    };
    m_leagues.push_back(ligue1);
}

void Database::resetStats() {
    for (auto& league : m_leagues) {
        for (auto& club : league.clubs) {
            club.points = 0;
            club.wins = 0;
            club.draws = 0;
            club.losses = 0;
            club.goalsFor = 0;
            club.goalsAgainst = 0;
        }
    }
}

const League* Database::getLeague(const std::string& name) const {
    for (const auto& l : m_leagues) {
        if (l.name == name) return &l;
    }
    return nullptr;
}

Club* Database::getClub(const std::string& leagueName, const std::string& clubName) {
    for (auto& l : m_leagues) {
        if (l.name == leagueName) {
            for (auto& c : l.clubs) {
                if (c.name == clubName) return &c;
            }
        }
    }
    return nullptr;
}
