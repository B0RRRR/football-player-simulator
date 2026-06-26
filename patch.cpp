#include "Database.h"
#include <vector>
#include <algorithm>

void generateEuropeanCups(Database& db) {
    std::vector<Club*> clClubs;
    std::vector<Club*> elClubs;
    
    std::vector<Club*> fourthPlaces;
    std::vector<Club*> seventhPlaces;
    
    // Process top 5 leagues (indices 0, 2, 4, 6, 8)
    const auto& leagues = db.getLeagues();
    for (size_t i = 0; i < leagues.size(); i += 2) {
        if (i + 1 >= leagues.size()) break;
        
        // We know processRelegation already sorted div1, but wait!
        // processRelegation modifies div1 by removing the bottom 3 and adding the top 3 of div2.
        // It does NOT re-sort div1 after adding the new clubs.
        // So the top 7 clubs of div1 are STILL the top 7 from the finished season!
        
        // Let's just grab the pointers. Since getLeagues() is const, we need mutable pointers.
        // We can just query by name.
        const League& l = leagues[i];
        
        for (int rank = 0; rank < 7 && rank < l.clubs.size(); ++rank) {
            Club* c = db.getClub(l.name, l.clubs[rank].name);
            if (!c) continue;
            
            if (rank < 3) {
                clClubs.push_back(c);
            } else if (rank == 3) {
                fourthPlaces.push_back(c);
            } else if (rank < 6) {
                elClubs.push_back(c);
            } else if (rank == 6) {
                seventhPlaces.push_back(c);
            }
        }
    }
    
    // Sort 4th places by points to get the best one
    std::sort(fourthPlaces.begin(), fourthPlaces.end(), [](Club* a, Club* b) {
        return a->points > b->points;
    });
    
    // Sort 7th places by points
    std::sort(seventhPlaces.begin(), seventhPlaces.end(), [](Club* a, Club* b) {
        return a->points > b->points;
    });
    
    if (!fourthPlaces.empty()) {
        clClubs.push_back(fourthPlaces[0]);
        for (size_t i = 1; i < fourthPlaces.size(); ++i) {
            elClubs.push_back(fourthPlaces[i]);
        }
    }
    
    if (seventhPlaces.size() >= 2) {
        elClubs.push_back(seventhPlaces[0]);
        elClubs.push_back(seventhPlaces[1]);
    }
    
    db.initTournaments(clClubs, elClubs);
}
