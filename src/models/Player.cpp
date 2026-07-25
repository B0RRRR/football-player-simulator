#include "Player.h"

Player::Player(const std::string& n) : name(n) {
    reset();
}

void Player::reset() {
    energy = 100;
    morale = 50;
    injuredDays = 0;
    suspensionMatches = 0;
    money = 0;
    salary = 1000;
    weeksPlayed = 0;
    // Real starting stats/potential are rolled per chosen position in NewCareerScreen;
    // these are only fallbacks for a Player made before that screen runs.
    shooting = 50;
    passing = 50;
    tackling = 50;
    goalkeeping = 50;
    potential = 88;
    goals = 0;
    assists = 0;
    experience = 0;
    position = PlayerPosition::Forward;
    currentClub = nullptr;
    age = 18;
    isCalledUp = false;
    totalSeasonRating = 0.0f;
    matchesPlayedThisSeason = 0;
    coachTrust = 50.0f;
    isTransferListed = false;
    contractYearsLeft = 3;
    achievements.clear();
}

int Player::overall() const {
    int sum = 0, n = 0;
    if (usesShooting())    { sum += shooting;    ++n; }
    if (usesPassing())     { sum += passing;     ++n; }
    if (usesTackling())    { sum += tackling;    ++n; }
    if (usesGoalkeeping()) { sum += goalkeeping; ++n; }
    return n ? sum / n : 0;
}

int Player::positionalRating() const {
    // Weights sum to 1.0 per row: {shooting, passing, tackling, goalkeeping}. Attributes
    // the position doesn't use carry zero weight.
    float s = 0.f, p = 0.f, t = 0.f, g = 0.f;
    switch (position) {
        case PlayerPosition::Forward:    s = 0.60f; p = 0.30f; t = 0.10f; break;
        case PlayerPosition::Midfielder: s = 0.25f; p = 0.45f; t = 0.30f; break;
        case PlayerPosition::Defender:   s = 0.15f; p = 0.30f; t = 0.55f; break;
        case PlayerPosition::Goalkeeper:            p = 0.15f; t = 0.20f; g = 0.65f; break;
    }
    return (int)(shooting * s + passing * p + tackling * t + goalkeeping * g + 0.5f);
}
