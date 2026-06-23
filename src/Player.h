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
    int shooting; // 1 to 100
    int passing; // 1 to 100
    int tackling; // 1 to 100
    int goalkeeping; // 1 to 100
    
    int goals;
    int assists;
    int experience;

    PlayerPosition position;
    Club* currentClub;
};
