#include "CareerManager.h"
#include "GameManager.h"
#include "Player.h"
#include "SaveManager.h"
#include <cstdlib>
#include <iostream>

CareerManager::CareerManager(GameManager* gm) : m_gameManager(gm), m_day(1) {
}

void CareerManager::resetCareer() {
    m_day = 1;
    m_gameManager->getDatabase().resetStats();
}

CalendarDayType CareerManager::getDayType() const {
    // 7 day cycle: Day 1,2,3 Training, Day 4 Rest, Day 5 Match, Day 6 Rest, Day 7 Rest
    int dayOfWeek = (m_day - 1) % 7;
    if (dayOfWeek >= 0 && dayOfWeek <= 2) return CalendarDayType::Training;
    if (dayOfWeek == 4) return CalendarDayType::Match;
    return CalendarDayType::Rest;
}

std::string CareerManager::getDayTypeString() const {
    if (hasEuropeanMatchToday()) return "European Match";
    switch (getDayType()) {
        case CalendarDayType::Training: return "Training";
        case CalendarDayType::Match: return "League Match";
        case CalendarDayType::Rest: return "Rest Day";
    }
    return "Unknown";
}

bool CareerManager::hasEuropeanMatchToday() const {
    // European matches are played on Day 3 of the week, every other week
    int week = (m_day - 1) / 7;
    int dayOfWeek = (m_day - 1) % 7;
    
    // Play on week 0, 2, 4... on day 2 (Wednesday equivalent)
    if (week % 2 == 0 && dayOfWeek == 2) {
        return true;
    }
    return false;
}

Club* CareerManager::getTodayOpponent() const {
    Player* p = m_gameManager->getPlayer();
    if (!p || !p->currentClub) return nullptr;
    
    if (hasEuropeanMatchToday()) {
        // Check Champions League
        Tournament& cl = m_gameManager->getDatabase().getChampionsLeague();
        if (!cl.isFinished && cl.currentRoundIndex < cl.rounds.size()) {
            for (const auto& m : cl.rounds[cl.currentRoundIndex].matches) {
                if (m.home == p->currentClub) return m.away;
                if (m.away == p->currentClub) return m.home;
            }
        }
        
        // Check Europa League
        Tournament& el = m_gameManager->getDatabase().getEuropaLeague();
        if (!el.isFinished && el.currentRoundIndex < el.rounds.size()) {
            for (const auto& m : el.rounds[el.currentRoundIndex].matches) {
                if (m.home == p->currentClub) return m.away;
                if (m.away == p->currentClub) return m.home;
            }
        }
        return nullptr; // Not participating or eliminated
    } else if (getDayType() == CalendarDayType::Match) {
        // League Match Opponent
        const League* lg = nullptr;
        for (const auto& l : m_gameManager->getDatabase().getLeagues()) {
            for (const auto& c : l.clubs) {
                if (c.name == p->currentClub->name) { lg = &l; break; }
            }
        }
        
        if (lg) {
            int n = static_cast<int>(lg->clubs.size());
            int r = p->weeksPlayed % (n - 1);
            int pIndex = -1;
            for (int i = 0; i < n; ++i) {
                if (lg->clubs[i].name == p->currentClub->name) { pIndex = i; break; }
            }
            
            auto rotate = [n, r](int x) {
                if (x == 0) return 0;
                return 1 + (x - 1 + r) % (n - 1);
            };
            
            for (int i = 0; i < n / 2; ++i) {
                int t1 = (i == 0) ? 0 : rotate(i);
                int t2 = rotate(n - 1 - i);
                if (t1 == pIndex) return m_gameManager->getDatabase().getClub(lg->name, lg->clubs[t2].name);
                if (t2 == pIndex) return m_gameManager->getDatabase().getClub(lg->name, lg->clubs[t1].name);
            }
        }
    }
    return nullptr;
}

bool CareerManager::isHomeMatchToday() const {
    Player* p = m_gameManager->getPlayer();
    if (!p || !p->currentClub) return true;
    
    if (hasEuropeanMatchToday()) {
        Tournament& cl = m_gameManager->getDatabase().getChampionsLeague();
        if (!cl.isFinished && cl.currentRoundIndex < cl.rounds.size()) {
            for (const auto& m : cl.rounds[cl.currentRoundIndex].matches) {
                if (m.home == p->currentClub) return !m.leg1Played;
                if (m.away == p->currentClub) return m.leg1Played;
            }
        }
        Tournament& el = m_gameManager->getDatabase().getEuropaLeague();
        if (!el.isFinished && el.currentRoundIndex < el.rounds.size()) {
            for (const auto& m : el.rounds[el.currentRoundIndex].matches) {
                if (m.home == p->currentClub) return !m.leg1Played;
                if (m.away == p->currentClub) return m.leg1Played;
            }
        }
        return true;
    } else {
        // Simple alternating logic for league
        return (p->weeksPlayed % 2 == 0);
    }
}

bool CareerManager::hasInternationalMatchToday() const {
    if (!m_isSummerBreak || m_summerDay % 3 != 0) return false;
    Player* p = m_gameManager->getPlayer();
    if (!p || !p->isCalledUp) return false;
    
    Database& db = m_gameManager->getDatabase();
    Tournament& t = (m_year % 2 == 0) ? db.getEuroCup() : db.getWorldCup();
    if (t.isFinished || t.currentRoundIndex >= t.rounds.size()) return false;
    
    for (const auto& m : t.rounds[t.currentRoundIndex].matches) {
        if (!m.leg1Played && (m.home->name == p->nationality || m.away->name == p->nationality)) {
            return true;
        }
    }
    return false;
}

bool CareerManager::hasRemainingInternationalMatches() const {
    if (!m_isSummerBreak) return false;
    Player* p = m_gameManager->getPlayer();
    if (!p || !p->isCalledUp) return false;
    
    Database& db = m_gameManager->getDatabase();
    Tournament& t = (m_year % 2 == 0) ? db.getEuroCup() : db.getWorldCup();
    if (t.isFinished || t.currentRoundIndex >= t.rounds.size()) return false;
    
    for (const auto& m : t.rounds[t.currentRoundIndex].matches) {
        if (m.home->name == p->nationality || m.away->name == p->nationality) {
            return true;
        }
    }
    return false;
}

Club* CareerManager::getInternationalOpponent() const {
    if (!hasInternationalMatchToday()) return nullptr;
    Player* p = m_gameManager->getPlayer();
    
    Database& db = m_gameManager->getDatabase();
    Tournament& t = (m_year % 2 == 0) ? db.getEuroCup() : db.getWorldCup();
    
    for (const auto& m : t.rounds[t.currentRoundIndex].matches) {
        if (!m.leg1Played) {
            if (m.home->name == p->nationality) return m.away;
            if (m.away->name == p->nationality) return m.home;
        }
    }
    return nullptr;
}

bool CareerManager::isHomeInternationalMatch() const {
    if (!hasInternationalMatchToday()) return true;
    Player* p = m_gameManager->getPlayer();
    
    Database& db = m_gameManager->getDatabase();
    Tournament& t = (m_year % 2 == 0) ? db.getEuroCup() : db.getWorldCup();
    
    for (const auto& m : t.rounds[t.currentRoundIndex].matches) {
        if (!m.leg1Played) {
            if (m.home->name == p->nationality) return true;
            if (m.away->name == p->nationality) return false;
        }
    }
    return true;
}

void CareerManager::simulateEuropeanMatches() {
    auto simMatches = [this](Tournament& t) {
        if (t.isFinished || t.currentRoundIndex >= t.rounds.size()) return;
        Player* p = m_gameManager->getPlayer();
        CupRound& round = t.rounds[t.currentRoundIndex];
        
        for (auto& m : round.matches) {
            // Don't simulate if it involves player
            if (p && p->currentClub && (m.home == p->currentClub || m.away == p->currentClub)) {
                continue; 
            }
            
            if (!m.leg1Played) {
                // Sim Leg 1
                int homeStr = m.home ? m.home->strength : 50;
                int awayStr = m.away ? m.away->strength : 50;
                m.homeGoalsLeg1 = rand() % 5;
                m.awayGoalsLeg1 = rand() % 4;
                if (homeStr > awayStr + 5) m.homeGoalsLeg1++;
                if (awayStr > homeStr + 5) m.awayGoalsLeg1++;
                m.leg1Played = true;
                
                if (m.isFinal) {
                    if (m.homeGoalsLeg1 > m.awayGoalsLeg1) m.winner = m.home;
                    else if (m.awayGoalsLeg1 > m.homeGoalsLeg1) m.winner = m.away;
                    else {
                        m.homePenalties = 4 + rand() % 2;
                        m.awayPenalties = 3 + rand() % 3;
                        if (m.homePenalties == m.awayPenalties) m.homePenalties++;
                        m.winner = (m.homePenalties > m.awayPenalties) ? m.home : m.away;
                    }
                }
            } else if (!m.leg2Played && !m.isFinal) {
                // Sim Leg 2
                int homeStr = m.home ? m.home->strength : 50;
                int awayStr = m.away ? m.away->strength : 50;
                m.homeGoalsLeg2 = rand() % 4; // home is actually away in leg 2
                m.awayGoalsLeg2 = rand() % 5;
                
                if (homeStr > awayStr + 5) m.homeGoalsLeg2++;
                if (awayStr > homeStr + 5) m.awayGoalsLeg2++;
                m.leg2Played = true;
                
                int agg1 = m.homeGoalsLeg1 + m.awayGoalsLeg2;
                int agg2 = m.awayGoalsLeg1 + m.homeGoalsLeg2;
                if (agg1 > agg2) m.winner = m.home;
                else if (agg2 > agg1) m.winner = m.away;
                else {
                    // Penalties
                    m.homePenalties = 4 + rand() % 2;
                    m.awayPenalties = 3 + rand() % 3;
                    if (m.homePenalties == m.awayPenalties) m.homePenalties++;
                    m.winner = (m.homePenalties > m.awayPenalties) ? m.home : m.away;
                }
            }
        }
        
        m_gameManager->getDatabase().advanceTournamentRound(t);
    };
    
    simMatches(m_gameManager->getDatabase().getChampionsLeague());
    simMatches(m_gameManager->getDatabase().getEuropaLeague());
}

void CareerManager::advanceDay() {
    Player* p = m_gameManager->getPlayer();
    
    if (m_isSummerBreak) {
        m_summerDay++;
        
        // Every 3 days play an international match round
        if (m_summerDay % 3 == 0) {
            simulateInternationalMatches();
        }
        
        Database& db = m_gameManager->getDatabase();
        Tournament& t = (m_year % 2 == 0) ? db.getEuroCup() : db.getWorldCup();
        
        if (t.isFinished || m_summerDay > 30) {
            endSummerBreak();
        }
        // Auto-save during summer break
        SaveManager::saveGame("savegame.json", m_gameManager->getPlayer(), this, &m_gameManager->getDatabase());
        return;
    }
    
    // Apply energy effects of the CURRENT day before advancing
    if (getDayType() == CalendarDayType::Training) {
        p->energy -= 15;
        if (p->energy < 0) p->energy = 0;
        // XP is now given by TrainingScreen
    } else if (getDayType() == CalendarDayType::Rest) {
        p->energy += 30;
        if (p->energy > 100) p->energy = 100;
    }
    
    if (p->injuredDays > 0) {
        p->injuredDays--;
    }
    
    // Match energy is drained during the match itself
    
    if (hasEuropeanMatchToday()) {
        // A European match day overrides the day type, so if the player doesn't have a match, it acts as a Rest day
        if (!getTodayOpponent()) {
            p->energy += 20;
            if (p->energy > 100) p->energy = 100;
        }
        simulateEuropeanMatches();
    } else if (getDayType() == CalendarDayType::Match) {
        simulateMatchweek();
    }
    // Advance internal day count
    m_day++;
    if (m_day % 7 == 0) {
        p->weeksPlayed++;
        p->money += p->salary;
    }
    
    // Auto-save after every day transition
    SaveManager::saveGame("savegame.json", m_gameManager->getPlayer(), this, &m_gameManager->getDatabase());
}

void CareerManager::skipSeason() {
    Player* p = m_gameManager->getPlayer();
    if (!p) return;
    
    while (p->weeksPlayed < getSeasonLength()) {
        if (hasEuropeanMatchToday()) {
            Club* opp = getTodayOpponent();
            if (opp) {
                // Simulate player's european match
                bool isHome = isHomeMatchToday();
                Club* hc = isHome ? p->currentClub : opp;
                Club* ac = isHome ? opp : p->currentClub;
                int hg = rand() % 4 + (hc->strength > ac->strength ? 1 : 0);
                int ag = rand() % 4 + (ac->strength > hc->strength ? 1 : 0);
                
                // We must update the tournament directly
                Tournament& cl = m_gameManager->getDatabase().getChampionsLeague();
                auto updateTournament = [&](Tournament& t) {
                    if (t.isFinished || t.currentRoundIndex >= t.rounds.size()) return;
                    for (auto& m : t.rounds[t.currentRoundIndex].matches) {
                        if ((m.home == hc && m.away == ac) || (m.home == ac && m.away == hc)) {
                            bool isHomeLeg = (m.home == hc);
                            if (!m.leg1Played) {
                                m.homeGoalsLeg1 = isHomeLeg ? hg : ag;
                                m.awayGoalsLeg1 = isHomeLeg ? ag : hg;
                                m.leg1Played = true;
                                if (m.isFinal) {
                                    if (hg > ag) m.winner = hc;
                                    else if (ag > hg) m.winner = ac;
                                    else {
                                        m.homePenalties = 4 + rand() % 2;
                                        m.awayPenalties = 3 + rand() % 3;
                                        if (m.homePenalties == m.awayPenalties) m.homePenalties++;
                                        m.winner = (m.homePenalties > m.awayPenalties) ? hc : ac;
                                    }
                                }
                            } else if (!m.leg2Played && !m.isFinal) {
                                m.homeGoalsLeg2 = isHomeLeg ? hg : ag;
                                m.awayGoalsLeg2 = isHomeLeg ? ag : hg;
                                m.leg2Played = true;
                                int aggHome = m.homeGoalsLeg1 + m.homeGoalsLeg2;
                                int aggAway = m.awayGoalsLeg1 + m.awayGoalsLeg2;
                                if (aggHome > aggAway) m.winner = m.home;
                                else if (aggAway > aggHome) m.winner = m.away;
                                else {
                                    m.homePenalties = 4 + rand() % 2;
                                    m.awayPenalties = 3 + rand() % 3;
                                    if (m.homePenalties == m.awayPenalties) m.homePenalties++;
                                    m.winner = (m.homePenalties > m.awayPenalties) ? m.home : m.away;
                                }
                            }
                        }
                    }
                };
                updateTournament(m_gameManager->getDatabase().getChampionsLeague());
                updateTournament(m_gameManager->getDatabase().getEuropaLeague());
            }
        } else if (getDayType() == CalendarDayType::Match) {
            Club* opp = nullptr;
            const League* lg = nullptr;
            for (const auto& l : m_gameManager->getDatabase().getLeagues()) {
                for (const auto& c : l.clubs) {
                    if (c.name == p->currentClub->name) { lg = &l; break; }
                }
            }
            if (lg) {
                int n = static_cast<int>(lg->clubs.size());
                int r = p->weeksPlayed % (n - 1);
                int pIndex = -1;
                for (int i = 0; i < n; ++i) {
                    if (lg->clubs[i].name == p->currentClub->name) { pIndex = i; break; }
                }
                auto rotate = [n, r](int x) { return x == 0 ? 0 : 1 + (x - 1 + r) % (n - 1); };
                for (int i = 0; i < n / 2; ++i) {
                    int t1 = (i == 0) ? 0 : rotate(i);
                    int t2 = rotate(n - 1 - i);
                    if (t1 == pIndex) { opp = m_gameManager->getDatabase().getClub(lg->name, lg->clubs[t2].name); break; }
                    else if (t2 == pIndex) { opp = m_gameManager->getDatabase().getClub(lg->name, lg->clubs[t1].name); break; }
                }
            }
            if (opp) {
                int hg = rand() % 4 + (p->currentClub->strength > opp->strength ? 1 : 0);
                int ag = rand() % 4 + (opp->strength > p->currentClub->strength ? 1 : 0);
                p->currentClub->goalsFor += hg; p->currentClub->goalsAgainst += ag;
                opp->goalsFor += ag; opp->goalsAgainst += hg;
                if (hg > ag) { p->currentClub->points += 3; p->currentClub->wins++; opp->losses++; }
                else if (ag > hg) { opp->points += 3; opp->wins++; p->currentClub->losses++; }
                else { p->currentClub->points += 1; p->currentClub->draws++; opp->points += 1; opp->draws++; }
                
                p->totalSeasonRating += (5.0f + (rand() % 50) / 10.0f);
                p->matchesPlayedThisSeason++;
            }
        } else if (getDayType() == CalendarDayType::Training) {
            p->experience += 50;
        }
        advanceDay();
    }
}

void CareerManager::simulateMatchweek() {
    Player* p = m_gameManager->getPlayer();
    if (!p || !p->currentClub) return;
    
    // Simulate matches for ALL leagues
    for (const auto& l : m_gameManager->getDatabase().getLeagues()) {
        int n = static_cast<int>(l.clubs.size());
        if (n < 2) continue;
        
        int numRounds = n - 1;
        int r = p->weeksPlayed % numRounds;
        
        // Find if the player is in this league
        int pIndex = -1;
        for (int i = 0; i < n; ++i) {
            if (l.clubs[i].name == p->currentClub->name) {
                pIndex = i;
                break;
            }
        }
        
        auto rotate = [n, r](int x) {
            if (x == 0) return 0;
            return 1 + (x - 1 + r) % (n - 1);
        };
        
        for (int i = 0; i < n / 2; ++i) {
            int t1 = (i == 0) ? 0 : rotate(i);
            int t2 = rotate(n - 1 - i);
            
            // Skip the match involving the player's club, it was already played
            if (t1 == pIndex || t2 == pIndex) continue;
            
            Club* c1 = m_gameManager->getDatabase().getClub(l.name, l.clubs[t1].name);
            Club* c2 = m_gameManager->getDatabase().getClub(l.name, l.clubs[t2].name);
            
            if (c1 && c2) {
                int chance1 = c1->strength + (rand() % 40 - 20);
                int chance2 = c2->strength + (rand() % 40 - 20);
                
                if (chance1 > chance2 + 10) {
                    // c1 wins
                    c1->points += 3; c1->wins++; c1->goalsFor += 2; c1->goalsAgainst += 0;
                    c2->losses++; c2->goalsFor += 0; c2->goalsAgainst += 2;
                } else if (chance2 > chance1 + 10) {
                    // c2 wins
                    c2->points += 3; c2->wins++; c2->goalsFor += 2; c2->goalsAgainst += 0;
                    c1->losses++; c1->goalsFor += 0; c1->goalsAgainst += 2;
                } else {
                    // draw
                    c1->points += 1; c1->draws++; c1->goalsFor += 1; c1->goalsAgainst += 1;
                    c2->points += 1; c2->draws++; c2->goalsFor += 1; c2->goalsAgainst += 1;
                }
            }
        }
    }
}

int CareerManager::getSeasonLength() const {
    Player* p = m_gameManager->getPlayer();
    if (!p || !p->currentClub) return 38;
    
    for (const auto& l : m_gameManager->getDatabase().getLeagues()) {
        for (const auto& c : l.clubs) {
            if (c.name == p->currentClub->name) {
                return (l.clubs.size() - 1) * 2;
            }
        }
    }
    return 38;
}

void CareerManager::endSeason() {
    Player* p = m_gameManager->getPlayer();
    std::string clubName = "";
    if (p) {
        p->age++;
        p->weeksPlayed = 0;
        if (p->currentClub) clubName = p->currentClub->name;
    }
    m_day = 1;
    
    // Pick European Cups Teams BEFORE relegation changes the clubs vectors
    std::vector<Club*> clClubs;
    std::vector<Club*> elClubs;
    std::vector<Club*> fourthPlaces;
    std::vector<Club*> seventhPlaces;
    
    Database& db = m_gameManager->getDatabase();
    const auto& leagues = db.getLeagues();
    for (size_t i = 0; i < leagues.size(); i += 2) {
        if (i + 1 >= leagues.size()) break;
        const League& l = leagues[i];
        for (int rank = 0; rank < 7 && rank < l.clubs.size(); ++rank) {
            Club* c = db.getClub(l.name, l.clubs[rank].name);
            if (!c) continue;
            if (rank < 3) clClubs.push_back(c);
            else if (rank == 3) fourthPlaces.push_back(c);
            else if (rank < 6) elClubs.push_back(c);
            else if (rank == 6) seventhPlaces.push_back(c);
        }
    }
    
    std::sort(fourthPlaces.begin(), fourthPlaces.end(), [](Club* a, Club* b) { return a->points > b->points; });
    std::sort(seventhPlaces.begin(), seventhPlaces.end(), [](Club* a, Club* b) { return a->points > b->points; });
    
    if (!fourthPlaces.empty()) {
        clClubs.push_back(fourthPlaces[0]);
        for (size_t i = 1; i < fourthPlaces.size(); ++i) elClubs.push_back(fourthPlaces[i]);
    }
    
    if (seventhPlaces.size() >= 2) {
        elClubs.push_back(seventhPlaces[0]);
        elClubs.push_back(seventhPlaces[1]);
    }
    // Archive club tournaments before resetting them
    db.archiveClubTournaments(m_year);
    
    db.initTournaments(clClubs, elClubs);
    
    // Now process relegation (this also archives the season)
    db.processRelegation(m_year);
    
    // Re-acquire player's current club pointer since vectors might have reallocated
    if (p && !clubName.empty()) {
        p->currentClub = db.getClub("", clubName);
    }
    
    db.resetStats();
    
    startSummerBreak();
}

void CareerManager::startSummerBreak() {
    m_isSummerBreak = true;
    m_summerDay = 1;
    m_year++;
    
    Player* p = m_gameManager->getPlayer();
    Database& db = m_gameManager->getDatabase();
    
    // Call-up logic
    float avgRating = 0.0f;
    if (p->matchesPlayedThisSeason > 0) {
        avgRating = p->totalSeasonRating / (float)p->matchesPlayedThisSeason;
    }
    
    if (avgRating > 7.0f && p->matchesPlayedThisSeason >= 10) {
        p->isCalledUp = true;
    } else {
        p->isCalledUp = false;
    }
    
    // Reset player season stats
    p->totalSeasonRating = 0.0f;
    p->matchesPlayedThisSeason = 0;
    
    // Generate Tournament
    if (m_year % 2 == 0) {
        db.generateEuroCup();
    } else {
        db.generateWorldCup();
    }
}

void CareerManager::endSummerBreak() {
    Database& db = m_gameManager->getDatabase();
    db.archiveInternationalTournaments(m_year);
    
    m_isSummerBreak = false;
    m_day = 1; // Start of regular season
}

void CareerManager::simulateInternationalMatches() {
    Database& db = m_gameManager->getDatabase();
    Tournament& t = (m_year % 2 == 0) ? db.getEuroCup() : db.getWorldCup();
    
    if (t.isFinished || t.currentRoundIndex >= t.rounds.size()) return;
    Player* p = m_gameManager->getPlayer();
    CupRound& round = t.rounds[t.currentRoundIndex];
    
    for (auto& m : round.matches) {
        // Skip player's match if they are called up and it's their team
        if (p && p->isCalledUp && (m.home->name == p->nationality || m.away->name == p->nationality)) {
            continue; 
        }
        
        if (!m.leg1Played) {
            int homeStr = m.home ? m.home->strength : 50;
            int awayStr = m.away ? m.away->strength : 50;
            m.homeGoalsLeg1 = rand() % 5;
            m.awayGoalsLeg1 = rand() % 4;
            if (homeStr > awayStr + 5) m.homeGoalsLeg1++;
            if (awayStr > homeStr + 5) m.awayGoalsLeg1++;
            m.leg1Played = true;
            
            if (m.homeGoalsLeg1 > m.awayGoalsLeg1) m.winner = m.home;
            else if (m.awayGoalsLeg1 > m.homeGoalsLeg1) m.winner = m.away;
            else {
                m.homePenalties = 4 + rand() % 2;
                m.awayPenalties = 3 + rand() % 3;
                if (m.homePenalties == m.awayPenalties) m.homePenalties++;
                m.winner = (m.homePenalties > m.awayPenalties) ? m.home : m.away;
            }
        }
    }
    
    db.advanceTournamentRound(t);
}

void CareerManager::skipSummer() {
    while (isSummerBreak()) {
        advanceDay();
    }
}

