#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <string>

struct Club;

// Pre-match team-sheet screen, shown after the player commits to a fixture and before the match
// itself kicks off. The screen is split home (left) vs away (right): crest, league position and
// recent form for each side, plus a Start Match button. It deliberately reserves a "tactics"
// strip so the formation / line-up UI can slot in here later without reworking the layout.
//
// No music plays here: the menu playlist fades out smoothly on entry (wantsMenuMusic == false)
// and the stadium ambience only starts once the real match screen is shown.
class MatchPreviewScreen : public Screen {
public:
    void init() override;
    void handleInput(sf::RenderWindow& window, const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

    bool wantsMenuMusic() const override { return false; }     // menu playlist fades out on entry
    bool wantsMatchAmbience() const override { return false; }  // ambience begins with the match

private:
    void resolveFixture();                       // work out today's home / away / user club
    int  leaguePosition(const Club* c) const;    // 1-based domestic rank, or 0 if not in a league
    void drawTeam(sf::RenderWindow& window, sf::Font& font, float cx,
                  const Club* c, bool isUser, sf::Color kit);

    Club* m_home = nullptr;
    Club* m_away = nullptr;
    Club* m_userClub = nullptr;
    bool  m_isNat = false;      // international fixture: crest lookup uses the flag set
    bool  m_valid = false;      // a fixture was resolved (otherwise we bail back to the hub)

    sf::FloatRect m_startBtn, m_backBtn;
    std::string m_pressed, m_hover; // "start" / "back"
};
