// Headless tests for MatchEngine. It takes plain Club*/Player* and needs no window, so the
// core simulation invariants can be checked without SFML. Tiny hand-rolled harness - no
// external test framework in this project. Build: cmake -B build -DBUILD_TESTS=ON.
#include "MatchEngine.h"
#include "Database.h"
#include "Player.h"

#include <cmath>
#include <iostream>
#include <string>

static int g_failures = 0;

#define CHECK(cond, msg)                                                          \
    do {                                                                          \
        if (!(cond)) { std::cerr << "FAIL: " << (msg) << "\n"; ++g_failures; }    \
        else { std::cout << "ok: " << (msg) << "\n"; }                            \
    } while (0)

// A player with everything set so he is picked to start (energy/trust high, fit, decent).
static Player makeStarter() {
    Player p("Tester");
    p.energy = 100; p.morale = 70; p.injuredDays = 0; p.suspensionMatches = 0;
    p.shooting = p.passing = p.tackling = p.goalkeeping = 70;
    p.position = PlayerPosition::Midfielder;
    return p;
}

// Fresh club pair for each engine (MatchEngine mutates club stats via commitEvent).
static void makeClubs(Club& home, Club& away) {
    home = Club{}; away = Club{};
    home.name = "Home FC"; home.strength = 70;
    away.name = "Away FC"; away.strength = 62;
}

static void testFullTimeReached() {
    Club home, away; makeClubs(home, away);
    Player p = makeStarter();
    MatchEngine e(&home, &away, true, &p);

    for (int i = 0; i < 200 && e.getState() != MatchState::Finished; ++i) e.updateMinute();

    CHECK(e.getState() == MatchState::Finished, "match reaches Finished");
    CHECK(e.getMinute() >= 90, "clock reaches 90'");

    // Extra ticks after full time must be inert - no runaway minute, no state flip.
    int m = e.getMinute();
    e.updateMinute();
    CHECK(e.getMinute() == m && e.getState() == MatchState::Finished,
          "updateMinute is a no-op once Finished");
}

static void testMomentumStaysBounded() {
    Club home, away; makeClubs(home, away);
    Player p = makeStarter();
    MatchEngine e(&home, &away, true, &p);

    // Drain the whole match, committing every event so momentum impulses fire too.
    for (int i = 0; i < 200 && e.getState() != MatchState::Finished; ++i) {
        e.updateMinute();
        while (e.hasLogs()) e.commitEvent(e.popRecentLog());
    }

    bool bounded = true;
    for (float v : e.getMomentumHistory())
        if (!(v >= -100.f && v <= 100.f) || std::isnan(v)) bounded = false;
    CHECK(bounded, "every momentum sample is within [-100, 100]");
}

static void testPossessionAddsTo100() {
    Club home, away; makeClubs(home, away);
    Player p = makeStarter();
    MatchEngine e(&home, &away, true, &p);
    for (int i = 0; i < 200 && e.getState() != MatchState::Finished; ++i) e.updateMinute();

    int mine = e.getPlayerTeamStats().possession;
    int theirs = e.getOpponentTeamStats().possession;
    CHECK(mine + theirs == 100, "both teams' possession sums to 100");
    CHECK(mine >= 20 && mine <= 80, "possession stays within the 20..80 cap");
}

static void testUserSentOffAndBanned() {
    Club home, away; makeClubs(home, away);
    Player p = makeStarter();               // midfielder -> user dot index 7
    MatchEngine e(&home, &away, true, &p);

    CHECK(e.userDotIndex() == 7, "midfielder maps to dot 7");
    CHECK(!e.isUserSubbedOff(), "user starts on the pitch");

    // A red carded onto the user's own dot must take him off and hand a two-match ban.
    MatchEvent card;
    card.type = EventType::Card; card.outcome = EventOutcome::RedCard; card.isHome = true;
    e.commitEvent(card);                    // records the red on the home side
    e.setLastRedCardPlayer(true, e.userDotIndex());

    CHECK(e.isUserSubbedOff(), "user is sent off when the red lands on his dot");
    CHECK(p.suspensionMatches == 2, "red card => two-match ban");
}

static void testSuspensionCountsDown() {
    Club home, away; makeClubs(home, away);
    Player p = makeStarter();
    p.suspensionMatches = 2;                 // banned coming into this fixture
    MatchEngine e(&home, &away, true, &p);   // sitting it out serves one match

    CHECK(!e.isUserSubbedOff() == false, "suspended player is not on the pitch");
    CHECK(p.suspensionMatches == 1, "sitting out a match burns one game of the ban");
}

int main() {
    testFullTimeReached();
    testMomentumStaysBounded();
    testPossessionAddsTo100();
    testUserSentOffAndBanned();
    testSuspensionCountsDown();

    if (g_failures) { std::cerr << "\n" << g_failures << " test(s) FAILED\n"; return 1; }
    std::cout << "\nAll engine tests passed\n";
    return 0;
}
