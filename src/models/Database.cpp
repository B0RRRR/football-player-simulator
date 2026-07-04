#include "Database.h"
#include <algorithm>
#include <random>

Database::Database() {
}

void Database::init() {
    // ================== ENGLAND ==================
    League epl;
    epl.name = "Premier League";
    epl.clubs = {
        {"Arsenal", 88}, {"Aston Villa", 81}, {"Bournemouth", 76}, {"Brighton & Hove Albion", 80},
        {"Brentford", 76}, {"Chelsea", 84}, {"Crystal Palace", 77}, {"Everton", 76},
        {"Fulham", 77}, {"Liverpool", 87}, {"Manchester City", 92}, {"Manchester United", 83},
        {"Newcastle United", 82}, {"Nottingham Forest", 75}, {"Sunderland", 73}, {"Tottenham Hotspur", 83},
        {"Leeds United", 74}, {"Coventry City", 70}, {"Ipswich Town", 72}, {"Hull City", 70}
    };
    m_leagues.push_back(epl);

    League champ;
    champ.name = "Championship";
    champ.clubs = {
        {"Blackburn Rovers", 67}, {"Bristol City", 69}, {"Cardiff City", 68}, {"Derby County", 68},
        {"Middlesbrough", 70}, {"Millwall", 67}, {"Norwich City", 71}, {"Plymouth Argyle", 66},
        {"Portsmouth", 65}, {"Preston North End", 69}, {"Queens Park Rangers", 67}, {"Southampton", 73},
        {"Stoke City", 67}, {"Swansea City", 68}, {"Watford", 68}, {"West Bromwich Albion", 71},
        {"Charlton Athletic", 65}, {"Wigan Athletic", 65}, {"Lincoln City", 64}, {"Bolton Wanderers", 66}
    };
    m_leagues.push_back(champ);

    // ================== SPAIN ==================
    League laliga;
    laliga.name = "La Liga";
    laliga.clubs = {
        {"Alaves", 76}, {"Athletic Bilbao", 82}, {"Atletico Madrid", 86}, {"Barcelona", 89},
        {"Celta Vigo", 75}, {"Deportivo La Coruna", 70}, {"Elche", 68}, {"Espanyol", 70},
        {"Getafe", 76}, {"Levante", 68}, {"Malaga", 69}, {"Osasuna", 77},
        {"Rayo Vallecano", 74}, {"Racing Santander", 68}, {"Real Betis", 81}, {"Real Madrid", 92},
        {"Real Sociedad", 82}, {"Villarreal", 80}, {"Valencia", 78}, {"Sevilla", 76}
    };
    m_leagues.push_back(laliga);

    League segunda;
    segunda.name = "Segunda Division";
    segunda.clubs = {
        {"Albacete", 65}, {"Almeria", 71}, {"Andorra", 63}, {"Burgos", 68},
        {"Cadiz", 73}, {"Castellon", 62}, {"Cordoba", 64}, {"Eibar", 70},
        {"Granada", 72}, {"Real Oviedo", 69}, {"Leganes", 71}, {"Girona", 83},
        {"Huesca", 68}, {"Racing de Ferrol", 68}, {"Sporting Gijon", 69}, {"Tenerife", 67},
        {"Zaragoza", 67}, {"Ceuta", 60}, {"Mallorca", 75}, {"Real Sociedad B", 65},
        {"Real Valladolid", 71}, {"Las Palmas", 75}
    };
    m_leagues.push_back(segunda);

    // ================== ITALY ==================
    League serieA;
    serieA.name = "Serie A";
    serieA.clubs = {
        {"Atalanta", 82}, {"Bologna", 82}, {"Cagliari", 74}, {"Como", 71},
        {"Monza", 77}, {"Fiorentina", 80}, {"Genoa", 76}, {"Inter", 88},
        {"Juventus", 84}, {"Lazio", 81}, {"Lecce", 75}, {"Milan", 85},
        {"Napoli", 81}, {"Parma", 72}, {"Frosinone", 73}, {"Roma", 82},
        {"Sassuolo", 75}, {"Torino", 78}, {"Udinese", 75}, {"Venezia", 71}
    };
    m_leagues.push_back(serieA);

    League serieB;
    serieB.name = "Serie B";
    serieB.clubs = {
        {"Avellino", 63}, {"Bari", 65}, {"Carrarese", 61}, {"Catanzaro", 69},
        {"Cesena", 64}, {"Cittadella", 66}, {"Empoli", 74}, {"Juve Stabia", 62},
        {"Mantova", 60}, {"Modena", 66}, {"Palermo", 69}, {"Padova", 65},
        {"Pescara", 66}, {"Reggiana", 67}, {"Sampdoria", 69}, {"Spezia", 66},
        {"Sudtirol", 68}, {"Verona", 74}, {"Pisa", 67}, {"Cremonese", 70}
    };
    m_leagues.push_back(serieB);

    // ================== GERMANY ==================
    League bundesliga;
    bundesliga.name = "Bundesliga";
    bundesliga.clubs = {
        {"Augsburg", 76}, {"Bayern Munich", 88}, {"Bayer Leverkusen", 87}, {"Borussia Dortmund", 84},
        {"Borussia Monchengladbach", 75}, {"Eintracht Frankfurt", 81}, {"Freiburg", 80}, {"Hoffenheim", 79},
        {"FC Koln", 74}, {"Mainz 05", 75}, {"RB Leipzig", 84}, {"Schalke 04", 69},
        {"Stuttgart", 83}, {"Union Berlin", 76}, {"Werder Bremen", 77}, {"Elversberg", 68},
        {"Paderborn", 69}, {"Hamburger SV", 71}
    };
    m_leagues.push_back(bundesliga);

    League bundesliga2;
    bundesliga2.name = "2. Bundesliga";
    bundesliga2.clubs = {
        {"Hannover 96", 70}, {"Darmstadt 98", 72}, {"Kaiserslautern", 67}, {"Hertha BSC", 70},
        {"Nurnberg", 68}, {"VfL Bochum", 74}, {"Karlsruher SC", 70}, {"Dynamo Dresden", 65},
        {"Holstein Kiel", 71}, {"Arminia Bielefeld", 66}, {"Magdeburg", 67}, {"Eintracht Braunschweig", 66},
        {"Greuther Furth", 69}, {"Fortuna Dusseldorf", 71}, {"Preussen Munster", 62}, {"VfL Wolfsburg", 76},
        {"Heidenheim", 77}, {"St. Pauli", 72}
    };
    m_leagues.push_back(bundesliga2);

    // ================== FRANCE ==================
    League ligue1;
    ligue1.name = "Ligue 1";
    ligue1.clubs = {
        {"Auxerre", 72}, {"Angers", 71}, {"Monaco", 82}, {"Troyes", 66},
        {"Lorient", 73}, {"Le Havre", 74}, {"Le Mans", 65}, {"Lille", 81},
        {"Nice", 80}, {"Lyon", 80}, {"Marseille", 79}, {"Paris FC", 69},
        {"Paris Saint-Germain", 88}, {"Lens", 79}, {"Brest", 81}, {"Rennes", 78},
        {"Strasbourg", 75}, {"Toulouse", 76}
    };
    m_leagues.push_back(ligue1);

    League ligue2;
    ligue2.name = "Ligue 2";
    ligue2.clubs = {
        {"Saint-Etienne", 71}, {"Clermont", 72}, {"Dijon", 68}, {"Guingamp", 68},
        {"Annecy", 65}, {"Metz", 73}, {"Nantes", 75}, {"Grenoble Foot", 67},
        {"Montpellier", 76}, {"Nancy", 65}, {"Pau", 67}, {"Red Star", 64},
        {"Rodez Aveyron", 69}, {"Sochaux", 66}, {"Laval", 68}, {"Reims", 77},
        {"Boulogne", 63}, {"Dunkerque", 65}
    };
    m_leagues.push_back(ligue2);
    
    initNationalTeams();
}

Club* Database::getClub(const std::string& leagueName, const std::string& clubName) {
    for (auto& league : m_leagues) {
        if (leagueName.empty() || league.name == leagueName) {
            for (auto& club : league.clubs) {
                if (club.name == clubName) {
                    return &club;
                }
            }
        }
    }
    // Also check national teams if not found in regular leagues
    if (leagueName.empty() || leagueName == m_nationalTeams.name) {
        for (auto& club : m_nationalTeams.clubs) {
            if (club.name == clubName) {
                return &club;
            }
        }
    }
    return nullptr;
}

void Database::archiveCurrentSeason(int year) {
    m_leagueHistory[year] = m_leagues;
}

void Database::archiveClubTournaments(int year) {
    m_championsLeagueHistory[year] = m_championsLeague;
    m_europaLeagueHistory[year] = m_europaLeague;
}

void Database::archiveInternationalTournaments(int year) {
    if (year % 2 == 0) {
        m_euroCupHistory[year] = m_euroCup;
    } else {
        m_worldCupHistory[year] = m_worldCup;
    }
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

void Database::processRelegation(int year) {
    // 1. Sort all leagues first
    for (auto& league : m_leagues) {
        std::sort(league.clubs.begin(), league.clubs.end(), [](const Club& a, const Club& b) {
            if (a.points != b.points) return a.points > b.points;
            int gdA = a.goalsFor - a.goalsAgainst;
            int gdB = b.goalsFor - b.goalsAgainst;
            if (gdA != gdB) return gdA > gdB;
            return a.goalsFor > b.goalsFor;
        });
    }

    // 2. Archive the properly sorted season before moving clubs
    archiveCurrentSeason(year);

    // 3. Move clubs
    for (size_t i = 0; i < m_leagues.size(); i += 2) {
        if (i + 1 >= m_leagues.size()) break;
        
        League& div1 = m_leagues[i];
        League& div2 = m_leagues[i+1];
        
        // Get bottom 3 of div1
        std::vector<Club> bottom3(div1.clubs.end() - 3, div1.clubs.end());
        div1.clubs.erase(div1.clubs.end() - 3, div1.clubs.end());
        
        // Get top 3 of div2
        std::vector<Club> top3(div2.clubs.begin(), div2.clubs.begin() + 3);
        div2.clubs.erase(div2.clubs.begin(), div2.clubs.begin() + 3);
        
        // Move bottom 3 to div2
        for (const auto& c : bottom3) div2.clubs.push_back(c);
        
        // Move top 3 to div1
        for (const auto& c : top3) div1.clubs.push_back(c);
    }
}

void Database::initTournaments(const std::vector<Club*>& clClubs, const std::vector<Club*>& elClubs) {
    m_championsLeague.name = "Champions League";
    m_championsLeague.rounds.clear();
    m_championsLeague.currentRoundIndex = 0;
    m_championsLeague.isFinished = false;
    m_championsLeague.winner = nullptr;
    
    m_europaLeague.name = "Europa League";
    m_europaLeague.rounds.clear();
    m_europaLeague.currentRoundIndex = 0;
    m_europaLeague.isFinished = false;
    m_europaLeague.winner = nullptr;
    
    auto createRounds = [](Tournament& t, const std::vector<Club*>& clubs) {
        if (clubs.size() != 16) return;
        
        CupRound r16;
        r16.name = "Round of 16";
        for (size_t i = 0; i < 8; ++i) {
            CupMatch m;
            m.home = clubs[i];
            m.away = clubs[15 - i]; // Simple seeding
            r16.matches.push_back(m);
        }
        t.rounds.push_back(r16);
        
        CupRound qf;
        qf.name = "Quarter-Finals";
        for (int i=0; i<4; ++i) qf.matches.push_back(CupMatch());
        t.rounds.push_back(qf);
        
        CupRound sf;
        sf.name = "Semi-Finals";
        for (int i=0; i<2; ++i) sf.matches.push_back(CupMatch());
        t.rounds.push_back(sf);
        
        CupRound f;
        f.name = "Final";
        CupMatch fm;
        fm.isFinal = true;
        f.matches.push_back(fm);
        t.rounds.push_back(f);
    };
    
    createRounds(m_championsLeague, clClubs);
    createRounds(m_europaLeague, elClubs);
}

void Database::advanceTournamentRound(Tournament& t) {
    if (t.isFinished || t.currentRoundIndex >= t.rounds.size()) return;
    
    CupRound& currentRound = t.rounds[t.currentRoundIndex];
    bool allFinished = true;
    for (auto& m : currentRound.matches) {
        if (!m.leg1Played || (!m.leg2Played && !m.isFinal)) {
            allFinished = false;
            break;
        }
    }
    
    if (allFinished) {
        currentRound.isCompleted = true;
        t.currentRoundIndex++;
        
        if (t.currentRoundIndex < t.rounds.size()) {
            // Populate next round
            CupRound& nextRound = t.rounds[t.currentRoundIndex];
            for (size_t i = 0; i < currentRound.matches.size(); i += 2) {
                if (i/2 < nextRound.matches.size()) {
                    nextRound.matches[i/2].home = currentRound.matches[i].winner;
                    if (i+1 < currentRound.matches.size()) {
                        nextRound.matches[i/2].away = currentRound.matches[i+1].winner;
                    }
                }
            }
        } else {
            t.isFinished = true;
            if (!currentRound.matches.empty()) {
                t.winner = currentRound.matches[0].winner;
            }
        }
    }
}

void Database::initNationalTeams() {
    m_nationalTeams.name = "National Teams";
    m_nationalTeams.clubs = {
        {"Argentina", 96}, {"Spain", 95}, {"France", 95}, {"England", 94},
        {"Portugal", 92}, {"Brazil", 93}, {"Morocco", 85}, {"Netherlands", 89},
        {"Belgium", 88}, {"Germany", 91}, {"Croatia", 86}, {"Italy", 89},
        {"Colombia", 85}, {"Mexico", 83}, {"Senegal", 82}, {"Uruguay", 86},
        {"USA", 81}, {"Japan", 82}, {"Switzerland", 84}, {"Iran", 78},
        {"Denmark", 84}, {"Turkey", 82}, {"Ecuador", 80}, {"Austria", 83},
        {"South Korea", 81}, {"Nigeria", 80}, {"Australia", 79}, {"Algeria", 79},
        {"Egypt", 78}, {"Canada", 78}, {"Norway", 82}, {"Russia", 79},
        {"Sweden", 81} // Added Sweden for Euro
    };
}

void Database::generateWorldCup() {
    m_worldCup.name = "World Cup";
    m_worldCup.rounds.clear();
    m_worldCup.currentRoundIndex = 0;
    m_worldCup.isFinished = false;
    m_worldCup.winner = nullptr;
    
    // Get all 32 teams except Sweden (index 32 is Sweden)
    std::vector<Club*> teams;
    for (int i = 0; i < 32 && i < m_nationalTeams.clubs.size(); ++i) {
        teams.push_back(&m_nationalTeams.clubs[i]);
    }
    
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(teams.begin(), teams.end(), g);
    
    CupRound r32;
    r32.name = "Round of 32";
    for (size_t i = 0; i < 16; ++i) {
        CupMatch m;
        m.home = teams[i * 2];
        m.away = teams[i * 2 + 1];
        m.isFinal = true; // Treats as 1-leg match with penalties!
        r32.matches.push_back(m);
    }
    m_worldCup.rounds.push_back(r32);
    
    CupRound r16; r16.name = "Round of 16";
    for (int i=0; i<8; ++i) { CupMatch m; m.isFinal = true; r16.matches.push_back(m); }
    m_worldCup.rounds.push_back(r16);
    
    CupRound qf; qf.name = "Quarter-Finals";
    for (int i=0; i<4; ++i) { CupMatch m; m.isFinal = true; qf.matches.push_back(m); }
    m_worldCup.rounds.push_back(qf);
    
    CupRound sf; sf.name = "Semi-Finals";
    for (int i=0; i<2; ++i) { CupMatch m; m.isFinal = true; sf.matches.push_back(m); }
    m_worldCup.rounds.push_back(sf);
    
    CupRound f; f.name = "Final";
    CupMatch fm; fm.isFinal = true; f.matches.push_back(fm);
    m_worldCup.rounds.push_back(f);
}

void Database::generateEuroCup() {
    m_euroCup.name = "Euro Cup";
    m_euroCup.rounds.clear();
    m_euroCup.currentRoundIndex = 0;
    m_euroCup.isFinished = false;
    m_euroCup.winner = nullptr;
    
    std::vector<std::string> euroNames = {
        "France", "Spain", "England", "Netherlands", "Portugal", "Belgium",
        "Germany", "Croatia", "Italy", "Switzerland", "Denmark", "Austria",
        "Norway", "Turkey", "Russia", "Sweden"
    };
    
    std::vector<Club*> teams;
    for (const auto& name : euroNames) {
        for (auto& c : m_nationalTeams.clubs) {
            if (c.name == name) {
                teams.push_back(&c);
                break;
            }
        }
    }
    
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(teams.begin(), teams.end(), g);
    
    CupRound r16; r16.name = "Round of 16";
    for (size_t i = 0; i < 8; ++i) {
        CupMatch m;
        m.home = teams[i * 2];
        m.away = teams[i * 2 + 1];
        m.isFinal = true; // 1-leg match
        r16.matches.push_back(m);
    }
    m_euroCup.rounds.push_back(r16);
    
    CupRound qf; qf.name = "Quarter-Finals";
    for (int i=0; i<4; ++i) { CupMatch m; m.isFinal = true; qf.matches.push_back(m); }
    m_euroCup.rounds.push_back(qf);
    
    CupRound sf; sf.name = "Semi-Finals";
    for (int i=0; i<2; ++i) { CupMatch m; m.isFinal = true; sf.matches.push_back(m); }
    m_euroCup.rounds.push_back(sf);
    
    CupRound f; f.name = "Final";
    CupMatch fm; fm.isFinal = true; f.matches.push_back(fm);
    m_euroCup.rounds.push_back(f);
}
