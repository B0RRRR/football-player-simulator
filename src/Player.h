#pragma once
#include <string>

class Player {
public:
    Player(const std::string& name);

    std::string name;
    int energy; // 0 to 100
    int shooting; // 1 to 100
    int passing; // 1 to 100
    
    int goals;
    int assists;
};
