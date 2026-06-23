#pragma once
#include <string>

class GameManager; // Forward declaration

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

    int getCurrentDay() const { return m_day; }
    CalendarDayType getDayType() const;
    std::string getDayTypeString() const;

private:
    GameManager* m_gameManager;
    int m_day;
};
