#pragma once
#include <string>

enum class PlayerPosition {
    Goalkeeper,
    Defender,
    Midfielder,
    Forward
};

struct Club; // Forward declaration

class Player {
public:
    Player(const std::string& name);

    std::string name;
    int energy; // 0 to 100
    int morale; // 0 to 100
    int injuredDays; // 0 if healthy

    int shooting; // 1 to 100
    int passing; // 1 to 100
    int tackling; // 1 to 100
    int goalkeeping; // 1 to 100
    
    int goals;
    int assists;
    int experience;

    PlayerPosition position;
    Club* currentClub = nullptr;
    int salary = 0; // Weekly salary
    int money = 0; // Accumulated money
    
    int age = 18; // Age of the player;
    int weeksPlayed;
};
