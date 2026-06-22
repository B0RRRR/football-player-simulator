#include "Player.h"

Player::Player(const std::string& n) : name(n) {
    energy = 100;
    shooting = 50; // 50% chance to score on shoot
    passing = 50;  // 50% chance to successfully pass
    goals = 0;
    assists = 0;
}
