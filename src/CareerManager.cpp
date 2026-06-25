#include "CareerManager.h"
#include "GameManager.h"
#include "Player.h"
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
    switch (getDayType()) {
        case CalendarDayType::Training: return "Training";
        case CalendarDayType::Match: return "Match Day";
        case CalendarDayType::Rest: return "Rest Day";
    }
    return "Unknown";
}

void CareerManager::advanceDay() {
    Player* p = m_gameManager->getPlayer();
    
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
    
    if (getDayType() == CalendarDayType::Match) {
        simulateMatchweek();
    }
    // Advance internal day count
    m_day++;
    if (m_day % 7 == 0) {
        p->weeksPlayed++;
        p->money += p->salary;
    }
}

void CareerManager::simulateMatchweek() {
    Player* p = m_gameManager->getPlayer();
    
    // Simple simulation for all clubs in the same league as the player
    // This is a naive random pairing just to populate the table.
    // In a real game, this would follow a strict schedule.
    
    // Note: The player's own match result should be recorded by MatchScreen.
    // So we don't simulate the player's club here, or if we do, we skip it.
    // For now, we skip simulating the league entirely here because we don't 
    // want to falsely simulate the player's match before they play it.
    // Actually, we can simulate all OTHER teams here.
    
    if (!p->currentClub) return;
    
    const League* league = m_gameManager->getDatabase().getLeague("Premier League"); // Fallback
    // Find the league the player is in
    for (const auto& l : m_gameManager->getDatabase().getLeagues()) {
        for (const auto& c : l.clubs) {
            if (c.name == p->currentClub->name) {
                league = &l;
                break;
            }
        }
    }
    
    if (!league) return;
    
    // We need mutable access to clubs to update their stats
    // Since getLeagues() returns const, we'll fetch them from the database
    // Wait, Database getClub gives mutable pointer. Let's just iterate over names.
    
    int n = league->clubs.size();
    if (n < 2) return;
    
    int numRounds = n - 1;
    int r = p->weeksPlayed % numRounds;
    
    int pIndex = -1;
    for (int i = 0; i < n; ++i) {
        if (league->clubs[i].name == p->currentClub->name) {
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
        
        Club* c1 = m_gameManager->getDatabase().getClub(league->name, league->clubs[t1].name);
        Club* c2 = m_gameManager->getDatabase().getClub(league->name, league->clubs[t2].name);
        
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
    
    m_gameManager->getDatabase().processRelegation();
    
    if (p && !clubName.empty()) {
        p->currentClub = m_gameManager->getDatabase().getClub("", clubName);
    }
    
    m_gameManager->getDatabase().resetStats();
}
