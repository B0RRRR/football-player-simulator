#pragma once
#include <string>

class GameManager;
struct Club;

enum class CalendarDayType {
    Training,
    Match,
    Rest
};

class CareerManager {
public:
    CareerManager(GameManager* gm);
    
    void resetCareer();
    void advanceDay();
    void simulateMatchweek(); // Simulate results for all other clubs
    void endSeason();
    int getSeasonLength() const;

    int getCurrentDay() const { return m_day; }
    CalendarDayType getDayType() const;
    std::string getDayTypeString() const;
    
    bool hasEuropeanMatchToday() const;
    Club* getTodayOpponent() const;
    bool isHomeMatchToday() const;
    
    void simulateEuropeanMatches();
    void skipSeason();

private:
    GameManager* m_gameManager;
    int m_day;
};
