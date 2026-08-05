#pragma once
#include <string>
#include <vector>

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
    // Wipe all career state back to a fresh 18-year-old prospect. Called when starting a
    // new career so nothing (age, money, coach trust, career goals...) leaks from the last.
    void reset();

    std::string name;
    int energy; // 0 to 100
    int morale; // 0 to 100
    int injuredDays; // 0 if healthy
    int suspensionMatches = 0; // 0 if not suspended

    int shooting; // 1 to 100
    int passing; // 1 to 100
    int tackling; // 1 to 100
    int goalkeeping; // 1 to 100
    int dribbling; // 1 to 100

    // The ceiling any single attribute can be trained to. Rolled once at career creation,
    // so a youngster can't max everything to 99 in a season - reaching even his own cap
    // takes several years of XP.
    int potential = 88;

    // Which attributes this position actually uses. An outfielder never keeps goal and a
    // keeper never shoots, so those stats aren't shown, aren't trainable, and don't count
    // towards his overall - otherwise a dead stat he can never raise would permanently
    // drag his rating down.
    bool usesShooting()    const { return position != PlayerPosition::Goalkeeper; }
    bool usesGoalkeeping() const { return position == PlayerPosition::Goalkeeper; }
    // Ball-carrying skill: everyone outfield dribbles; a keeper doesn't take players on.
    bool usesDribbling()   const { return position != PlayerPosition::Goalkeeper; }
    bool usesPassing()     const { return true; } // everyone passes
    // Keepers keep tackling: it's their sweeping / rushing off the line / one-on-ones. It
    // also keeps every position on three trainable attributes, so no role reaches a high
    // overall faster than another simply by having fewer stats to split XP between.
    bool usesTackling()    const { return true; }

    // Average of the attributes that matter for his position - the headline "overall".
    int overall() const;

    // How good he is AT HIS JOB. Weighted by position, so a striker isn't judged on his
    // goalkeeping. This is what team selection uses - a specialist plays even while his
    // off-position stats are low. Weights per position sum to 1.
    int positionalRating() const;

    int goals;   // THIS season (reset each summer, like AI squad-mates)
    int assists; // THIS season
    int careerGoals = 0;   // lifetime total, accumulated at season end
    int careerAssists = 0;
    int experience;

    PlayerPosition position;
    Club* currentClub = nullptr;
    int salary = 0; // Weekly salary
    int money = 0; // Accumulated money
    
    int age = 18; // Age of the player;
    int weeksPlayed = 0;
    
    std::string nationality;
    bool isCalledUp = false;
    
    float totalSeasonRating = 0.0f;
    int matchesPlayedThisSeason = 0;
    float coachTrust = 50.0f; // 0 to 100
    
    bool isTransferListed = false;
    int contractYearsLeft = 3;
    
    std::vector<std::string> achievements;
};
