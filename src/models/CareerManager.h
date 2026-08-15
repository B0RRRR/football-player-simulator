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
    
    static void distributeGoalsToRoster(Club* club, int goals);
    static void updateAITeamMatchStats(Club* club);
    
    void resetCareer();
    void advanceDay(bool simulatePlayerClub = false, bool autoSave = true);
    void simulateMatchweek(bool simulatePlayerClub = false); // Simulate results for all other clubs
    void endSeason();
    int getSeasonLength() const;
    int getYear() const { return m_year; }
    void setYear(int y) { m_year = y; }

    int getCurrentDay() const { return m_day; }
    void setCurrentDay(int d) { m_day = d; }
    CalendarDayType getDayType() const;
    std::string getDayTypeString() const;
    // Today's calendar date, e.g. "12 Nov 2024" (or "12 Nov" when withYear is false).
    std::string dateString(bool withYear = true) const;
    // Format an arbitrary club-season day (1-based) on the same calendar.
    std::string dateStringForDay(int day, bool withYear = true) const;
    // Planned date of a European cup round's leg (roundIndex 0=R16..3=Final), known before it's
    // played from the fixed fixture schedule. Empty for a leg that doesn't exist (Final leg 2).
    std::string europeanRoundDate(int roundIndex, bool secondLeg) const;
    
    bool hasEuropeanMatchToday() const;
    Club* getTodayOpponent() const;
    bool isHomeMatchToday() const;
    
    bool hasRemainingInternationalMatches() const;
    
    bool hasInternationalMatchToday() const;
    Club* getInternationalOpponent() const;
    bool isHomeInternationalMatch() const;
    
    // Seed the Champions/Europa League for the opening season, when there are no standings
    // yet - the entrants are the strongest clubs of each top division.
    void seedEuropeanCupsByStrength();
    void simulateEuropeanMatches(bool simulatePlayerClub = false);
    void simulateInternationalMatches(bool simulatePlayerClub = false);
    void skipSeason();
    void skipSummer();
    
    bool isSummerBreak() const { return m_isSummerBreak; }
    void setSummerBreak(bool b) { m_isSummerBreak = b; }
    int getSummerDay() const { return m_summerDay; }
    void setSummerDay(int d) { m_summerDay = d; }
    void startSummerBreak();
    void endSummerBreak();

private:
    GameManager* m_gameManager;
    int m_day;
    bool m_isSummerBreak = false;
    int m_summerDay = 0;
    int m_year = 2024; // Track years to alternate World Cup / Euros
};
