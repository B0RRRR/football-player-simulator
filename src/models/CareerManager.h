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
    int getYear() const { return m_year; }

    int getCurrentDay() const { return m_day; }
    CalendarDayType getDayType() const;
    std::string getDayTypeString() const;
    
    bool hasEuropeanMatchToday() const;
    Club* getTodayOpponent() const;
    bool isHomeMatchToday() const;
    
    bool hasRemainingInternationalMatches() const;
    
    bool hasInternationalMatchToday() const;
    Club* getInternationalOpponent() const;
    bool isHomeInternationalMatch() const;
    
    void simulateEuropeanMatches();
    void simulateInternationalMatches();
    void skipSeason();
    void skipSummer();
    
    bool isSummerBreak() const { return m_isSummerBreak; }
    void startSummerBreak();
    void endSummerBreak();

private:
    GameManager* m_gameManager;
    int m_day;
    bool m_isSummerBreak = false;
    int m_summerDay = 0;
    int m_year = 2024; // Track years to alternate World Cup / Euros
};
