#include "Player.h"

Player::Player(const std::string& n) : name(n) {
    energy = 100;
    morale = 50;
    injuredDays = 0;
    money = 0;
    salary = 1000;
    weeksPlayed = 0;
    shooting = 65; // 65% chance to score on shoot
    passing = 65;  // 65% chance to successfully pass
    tackling = 65;
    goalkeeping = 65;
    goals = 0;
    assists = 0;
    experience = 0;
    position = PlayerPosition::Forward;
    currentClub = nullptr;
}
