#include "Database.h"
#include <algorithm>

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
        {"Oviedo CF", 68}, {"Racing de Ferrol", 68}, {"Sporting Gijon", 69}, {"Tenerife", 67},
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
    return nullptr;
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

void Database::processRelegation() {
    // Pairs: 0-1, 2-3, 4-5, 6-7, 8-9
    for (size_t i = 0; i < m_leagues.size(); i += 2) {
        if (i + 1 >= m_leagues.size()) break;
        
        League& div1 = m_leagues[i];
        League& div2 = m_leagues[i+1];
        
        // Sort div1 (descending)
        std::sort(div1.clubs.begin(), div1.clubs.end(), [](const Club& a, const Club& b) {
            if (a.points != b.points) return a.points > b.points;
            int gdA = a.goalsFor - a.goalsAgainst;
            int gdB = b.goalsFor - b.goalsAgainst;
            if (gdA != gdB) return gdA > gdB;
            return a.goalsFor > b.goalsFor;
        });
        
        // Sort div2 (descending)
        std::sort(div2.clubs.begin(), div2.clubs.end(), [](const Club& a, const Club& b) {
            if (a.points != b.points) return a.points > b.points;
            int gdA = a.goalsFor - a.goalsAgainst;
            int gdB = b.goalsFor - b.goalsAgainst;
            if (gdA != gdB) return gdA > gdB;
            return a.goalsFor > b.goalsFor;
        });
        
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
