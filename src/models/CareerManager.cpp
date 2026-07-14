#include "CareerManager.h"
#include "GameManager.h"
#include "Player.h"
#include "SaveManager.h"
#include <cstdlib>
#include <iostream>
#include <algorithm>

void CareerManager::distributeGoalsToRoster(Club* club, int goals) {
    if (!club || club->roster.empty() || goals == 0) return;
    
    for (int i = 0; i < goals; ++i) {
        // Find a random player to score, weighted towards FWD and MID
        std::vector<AIPlayer*> candidates;
        for (auto p : club->roster) {
            if (p->position == PlayerPosition::Forward) {
                for (int w=0; w<5; ++w) candidates.push_back(p);
            } else if (p->position == PlayerPosition::Midfielder) {
                for (int w=0; w<3; ++w) candidates.push_back(p);
            } else if (p->position == PlayerPosition::Defender) {
                candidates.push_back(p);
            }
        }
        if (!candidates.empty()) {
            AIPlayer* scorer = candidates[rand() % candidates.size()];
            scorer->goals++;
            
            // 70% chance for an assist
            if (rand() % 100 < 70) {
                AIPlayer* assister = candidates[rand() % candidates.size()];
                if (assister != scorer) assister->assists++;
            }
        }
    }
}

void CareerManager::updateAITeamMatchStats(Club* club) {
    if (!club || club->roster.empty()) return;
    
    // Pick 11 random players to represent the starting XI
    std::vector<AIPlayer*> squad = club->roster;
    std::random_shuffle(squad.begin(), squad.end()); // Basic shuffle
    
    for (int i = 0; i < 11 && i < squad.size(); ++i) {
        AIPlayer* p = squad[i];
        p->matchesPlayed++;
        
        // Generate a random match rating (e.g. 5.0 to 9.0), slightly weighted by overall
        float baseRating = 5.0f + (p->overall - 40) / 15.0f; 
        float matchRating = baseRating + ((rand() % 30) - 10) / 10.0f; 
        matchRating = std::clamp(matchRating, 3.0f, 10.0f);
        
        // Calculate rolling average
        p->avgRating = ((p->avgRating * (p->matchesPlayed - 1)) + matchRating) / p->matchesPlayed;
    }
}

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
    // European matches are played on Day 3 of the week
    int week = (m_day - 1) / 7;
    int dayOfWeek = (m_day - 1) % 7;
    
    // Play on specific weeks to spread the tournament properly
    if (dayOfWeek == 2) {
        if (week == 10 || week == 12 || week == 20 || week == 22 || 
            week == 30 || week == 32 || week == 36) {
            return true;
        }
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

void CareerManager::simulateEuropeanMatches(bool simulatePlayerClub) {
    auto simMatches = [this, simulatePlayerClub](Tournament& t) {
        if (t.isFinished || t.currentRoundIndex >= t.rounds.size()) return;
        Player* p = m_gameManager->getPlayer();
        CupRound& round = t.rounds[t.currentRoundIndex];
        
        for (auto& m : round.matches) {
            // Don't simulate if it involves player and simulatePlayerClub is false
            if (p && p->currentClub && (m.home == p->currentClub || m.away == p->currentClub)) {
                if (!simulatePlayerClub) continue; 
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
                
                distributeGoalsToRoster(m.home, m.homeGoalsLeg1);
                distributeGoalsToRoster(m.away, m.awayGoalsLeg1);
                updateAITeamMatchStats(m.home);
                updateAITeamMatchStats(m.away);
                
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
                
                distributeGoalsToRoster(m.home, m.homeGoalsLeg2);
                distributeGoalsToRoster(m.away, m.awayGoalsLeg2);
                updateAITeamMatchStats(m.home);
                updateAITeamMatchStats(m.away);
                
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

void CareerManager::advanceDay(bool simulatePlayerClub, bool autoSave) {
    Player* p = m_gameManager->getPlayer();
    
    if (m_isSummerBreak) {
        m_summerDay++;
        
        // Every 3 days play an international match round
        if (m_summerDay % 3 == 0) {
            simulateInternationalMatches(simulatePlayerClub);
        }
        
        Database& db = m_gameManager->getDatabase();
        Tournament& t = (m_year % 2 == 0) ? db.getEuroCup() : db.getWorldCup();
        
        if (t.isFinished || m_summerDay > 30) {
            endSummerBreak();
        }
        // Auto-save during summer break
        if (autoSave) SaveManager::saveGame("savegame.json", m_gameManager->getPlayer(), this, &m_gameManager->getDatabase());
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
        simulateEuropeanMatches(simulatePlayerClub);
    } else if (getDayType() == CalendarDayType::Match) {
        simulateMatchweek(simulatePlayerClub);
    }
    // Advance internal day count
    m_day++;
    if (m_day % 7 == 0) {
        p->weeksPlayed++;
        p->money += p->salary;
    }
    
    // Auto-save after every day transition
    if (autoSave) SaveManager::saveGame("savegame.json", m_gameManager->getPlayer(), this, &m_gameManager->getDatabase());
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
                                
                                distributeGoalsToRoster(m.home, m.homeGoalsLeg1);
                                distributeGoalsToRoster(m.away, m.awayGoalsLeg1);
                                updateAITeamMatchStats(m.home);
                                updateAITeamMatchStats(m.away);
                                
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
                                
                                distributeGoalsToRoster(m.home, m.homeGoalsLeg2);
                                distributeGoalsToRoster(m.away, m.awayGoalsLeg2);
                                updateAITeamMatchStats(m.home);
                                updateAITeamMatchStats(m.away);
                                
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
                
                distributeGoalsToRoster(p->currentClub, hg);
                distributeGoalsToRoster(opp, ag);
                updateAITeamMatchStats(p->currentClub);
                updateAITeamMatchStats(opp);
                
                if (hg > ag) { p->currentClub->points += 3; p->currentClub->wins++; opp->losses++; }
                else if (ag > hg) { opp->points += 3; opp->wins++; p->currentClub->losses++; }
                else { p->currentClub->points += 1; p->currentClub->draws++; opp->points += 1; opp->draws++; }
                
                p->totalSeasonRating += (5.0f + (rand() % 50) / 10.0f);
                p->matchesPlayedThisSeason++;
            }
        } else if (getDayType() == CalendarDayType::Training) {
            p->experience += 50;
        }
        advanceDay(false, false);
    }
    SaveManager::saveGame("savegame.json", m_gameManager->getPlayer(), this, &m_gameManager->getDatabase());
}

void CareerManager::simulateMatchweek(bool simulatePlayerClub) {
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
            
            // Skip the match involving the player's club, it was already played (unless forced)
            if (t1 == pIndex || t2 == pIndex) {
                if (!simulatePlayerClub) continue;
            }
            
            Club* c1 = m_gameManager->getDatabase().getClub(l.name, l.clubs[t1].name);
            Club* c2 = m_gameManager->getDatabase().getClub(l.name, l.clubs[t2].name);
            
            if (c1 && c2) {
                int diff = c1->strength - c2->strength;
                int g1 = 0, g2 = 0;
                
                // Base goals distribution
                int r1 = rand() % 100;
                if (r1 < 30) g1 = 0;
                else if (r1 < 65) g1 = 1;
                else if (r1 < 85) g1 = 2;
                else g1 = 3 + rand() % 3;
                
                int r2 = rand() % 100;
                if (r2 < 40) g2 = 0; // away team scores slightly less
                else if (r2 < 75) g2 = 1;
                else if (r2 < 92) g2 = 2;
                else g2 = 3 + rand() % 2;
                
                // Adjust based on strength difference
                if (diff > 5) g1++;
                if (diff > 15) g1++;
                if (diff < -5) g2++;
                if (diff < -15) g2++;
                
                c1->goalsFor += g1; c1->goalsAgainst += g2;
                c2->goalsFor += g2; c2->goalsAgainst += g1;
                
                distributeGoalsToRoster(c1, g1);
                distributeGoalsToRoster(c2, g2);
                updateAITeamMatchStats(c1);
                updateAITeamMatchStats(c2);
                
                if (g1 > g2) {
                    c1->points += 3; c1->wins++; c2->losses++;
                } else if (g2 > g1) {
                    c2->points += 3; c2->wins++; c1->losses++;
                } else {
                    c1->points += 1; c1->draws++; c2->points += 1; c2->draws++;
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
    
    // Process AI Players stats
    for (const auto& aip : db.getPlayers()) {
        if (aip) {
            aip->age++;
            aip->careerGoals += aip->goals;
            aip->careerAssists += aip->assists;
            aip->careerMatches += aip->matchesPlayed;
            
            aip->goals = 0;
            aip->assists = 0;
            aip->matchesPlayed = 0;
            aip->avgRating = 0.0f;
            
            // Progression / Regression
            if (aip->age < 24) {
                if (rand() % 100 < 70) aip->overall += (1 + rand() % 3);
            } else if (aip->age > 30) {
                if (rand() % 100 < 60) aip->overall -= (1 + rand() % 2);
            }
            aip->overall = std::clamp(aip->overall, 30, 99);
        }
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
    
    int overall = (p->shooting + p->passing + p->tackling + p->goalkeeping) / 4;
    
    if (avgRating >= 7.0f && p->matchesPlayedThisSeason >= 20 && overall >= 75) {
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

void CareerManager::simulateInternationalMatches(bool simulatePlayerClub) {
    Database& db = m_gameManager->getDatabase();
    Tournament& t = (m_year % 2 == 0) ? db.getEuroCup() : db.getWorldCup();
    
    if (t.isFinished || t.currentRoundIndex >= t.rounds.size()) return;
    Player* p = m_gameManager->getPlayer();
    CupRound& round = t.rounds[t.currentRoundIndex];
    
    for (auto& m : round.matches) {
        // Skip player's match if they are called up and it's their team (unless forced)
        if (p && p->isCalledUp && (m.home->name == p->nationality || m.away->name == p->nationality)) {
            if (!simulatePlayerClub) continue; 
        }
        
        if (!m.leg1Played) {
            int homeStr = m.home ? m.home->strength : 50;
            int awayStr = m.away ? m.away->strength : 50;
            m.homeGoalsLeg1 = rand() % 5;
            m.awayGoalsLeg1 = rand() % 4;
            if (homeStr > awayStr + 5) m.homeGoalsLeg1++;
            if (awayStr > homeStr + 5) m.awayGoalsLeg1++;
            m.leg1Played = true;
            
            distributeGoalsToRoster(m.home, m.homeGoalsLeg1);
            distributeGoalsToRoster(m.away, m.awayGoalsLeg1);
            updateAITeamMatchStats(m.home);
            updateAITeamMatchStats(m.away);
            
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
        advanceDay(false, false);
    }
    SaveManager::saveGame("savegame.json", m_gameManager->getPlayer(), this, &m_gameManager->getDatabase());
}

