#include "UITheme.h"
#include "UIKit.h"
#include "MatchStatsScreen.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "CareerManager.h"
#include "AssetManager.h"
#include <algorithm>
#include <cstdio>

MatchStatsScreen::MatchStatsScreen(std::shared_ptr<MatchEngine> engine)
    : m_engine(engine) {}

void MatchStatsScreen::init() {
    const MatchStats& home = m_engine->isHome() ? m_engine->getPlayerTeamStats() : m_engine->getOpponentTeamStats();
    const MatchStats& away = m_engine->isHome() ? m_engine->getOpponentTeamStats() : m_engine->getPlayerTeamStats();

    m_hName = m_engine->isHome() ? m_engine->getPlayerClub()->name : m_engine->getOpponentClub()->name;
    m_aName = !m_engine->isHome() ? m_engine->getPlayerClub()->name : m_engine->getOpponentClub()->name;
    m_isNat = m_gameManager->getCareerManager()->hasInternationalMatchToday();

    m_hg = home.goals; m_ag = away.goals;
    m_hs = home.shots; m_as = away.shots;
    m_hy = home.yellowCards; m_ay = away.yellowCards;
    m_hr = home.redCards;    m_ar = away.redCards;

    // Distribute AI goals.
    int userGoals = m_engine->getUserGoalsScored();
    int homeAIGoals = home.goals - (m_engine->isHome() ? userGoals : 0);
    int awayAIGoals = away.goals - (!m_engine->isHome() ? userGoals : 0);

    Club* hc = m_engine->isHome() ? m_engine->getPlayerClub() : m_engine->getOpponentClub();
    Club* ac = m_engine->isHome() ? m_engine->getOpponentClub() : m_engine->getPlayerClub();

    if (homeAIGoals > 0) m_gameManager->getCareerManager()->distributeGoalsToRoster(hc, homeAIGoals);
    if (awayAIGoals > 0) m_gameManager->getCareerManager()->distributeGoalsToRoster(ac, awayAIGoals);
    m_gameManager->getCareerManager()->updateAITeamMatchStats(hc);
    m_gameManager->getCareerManager()->updateAITeamMatchStats(ac);

    // Player rating -> XP / trust.
    m_rating = m_engine->getPlayerRating();
    Player* p = m_gameManager->getPlayer();
    if (p) {
        if (p->coachTrust < 30.0f) {
            m_benched = true;
        } else {
            p->totalSeasonRating += m_rating;
            p->matchesPlayedThisSeason++;
            int matchXp = 30 + (int)((m_rating - 5.0f) * 25.0f);
            if (matchXp < 30) matchXp = 30;
            matchXp += m_engine->getUserGoalsScored() * 25;
            p->experience += matchXp;
            m_xpGain = matchXp;

            float trustChange = m_rating >= 6.5f ? 5.0f : (m_rating < 6.0f ? -5.0f : 0.0f);
            p->coachTrust = std::clamp(p->coachTrust + trustChange, 0.f, 100.f);
            m_trustGain = (int)trustChange;
        }
    }

    m_continueBtn = sf::FloatRect(540.f, 616.f, 200.f, 54.f);

    // League points / cup progression (unchanged logic).
    if (m_gameManager->getCareerManager()->hasInternationalMatchToday()) {
        auto updateTournament = [&](Tournament& t) {
            if (t.isFinished || t.currentRoundIndex >= t.rounds.size()) return;
            for (auto& m : t.rounds[t.currentRoundIndex].matches) {
                if ((m.home == hc && m.away == ac) || (m.home == ac && m.away == hc)) {
                    bool isHomeLeg = (m.home == hc);
                    if (!m.leg1Played) {
                        m.homeGoalsLeg1 = isHomeLeg ? home.goals : away.goals;
                        m.awayGoalsLeg1 = isHomeLeg ? away.goals : home.goals;
                        m.leg1Played = true;
                        m.winner = (home.goals > away.goals) ? hc : ((away.goals > home.goals) ? ac : nullptr);
                        if (!m.winner) {
                            m.homePenalties = 4 + rand() % 2;
                            m.awayPenalties = 3 + rand() % 3;
                            if (m.homePenalties == m.awayPenalties) m.homePenalties++;
                            m.winner = (m.homePenalties > m.awayPenalties) ? m.home : m.away;
                            m_penaltyNote = "Tied! " + m.winner->name + " won on penalties.";
                        }
                    }
                }
            }
        };
        updateTournament(m_gameManager->getDatabase().getWorldCup());
        updateTournament(m_gameManager->getDatabase().getEuroCup());
    } else if (m_gameManager->getCareerManager()->hasEuropeanMatchToday()) {
        auto updateTournament = [&](Tournament& t) {
            if (t.isFinished || t.currentRoundIndex >= t.rounds.size()) return;
            for (auto& m : t.rounds[t.currentRoundIndex].matches) {
                if ((m.home == hc && m.away == ac) || (m.home == ac && m.away == hc)) {
                    bool isHomeLeg = (m.home == hc);
                    if (!m.leg1Played) {
                        m.homeGoalsLeg1 = isHomeLeg ? home.goals : away.goals;
                        m.awayGoalsLeg1 = isHomeLeg ? away.goals : home.goals;
                        m.leg1Played = true;
                        if (m.isFinal) {
                            m.winner = (home.goals > away.goals) ? hc : ((away.goals > home.goals) ? ac : nullptr);
                            if (!m.winner) m.winner = (rand() % 2 == 0) ? hc : ac;
                        }
                    } else if (!m.leg2Played && !m.isFinal) {
                        m.homeGoalsLeg2 = isHomeLeg ? home.goals : away.goals;
                        m.awayGoalsLeg2 = isHomeLeg ? away.goals : home.goals;
                        m.leg2Played = true;
                        int aggHome = m.homeGoalsLeg1 + m.homeGoalsLeg2;
                        int aggAway = m.awayGoalsLeg1 + m.awayGoalsLeg2;
                        if (aggHome > aggAway) m.winner = m.home;
                        else if (aggAway > aggHome) m.winner = m.away;
                        else {
                            m.homePenalties = 4 + rand() % 2;
                            m.awayPenalties = 3 + rand() % 3;
                            if (m.homePenalties == m.awayPenalties) m.homePenalties++;
                            m.winner = (m.homePenalties > m.awayPenalties) ? m.home : m.away;
                            m_penaltyNote = "Aggregate tied! " + m.winner->name + " won on penalties.";
                        }
                    }
                }
            }
        };
        updateTournament(m_gameManager->getDatabase().getChampionsLeague());
        updateTournament(m_gameManager->getDatabase().getEuropaLeague());
    } else {
        hc->goalsFor += home.goals; hc->goalsAgainst += away.goals;
        ac->goalsFor += away.goals; ac->goalsAgainst += home.goals;
        if (home.goals > away.goals)      { hc->points += 3; hc->wins++;  ac->losses++; }
        else if (away.goals > home.goals) { ac->points += 3; ac->wins++;  hc->losses++; }
        else                              { hc->points += 1; hc->draws++; ac->points += 1; ac->draws++; }
    }
}

void MatchStatsScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseMoved)
        m_hover = m_continueBtn.contains(window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y}));
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        m_pressed = m_continueBtn.contains(window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y}));
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        if (m_pressed && m_continueBtn.contains(window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y}))) {
            m_gameManager->getCareerManager()->advanceDay();
            m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
        }
        m_pressed = false;
    }
}

void MatchStatsScreen::update(sf::Time) {}

void MatchStatsScreen::draw(sf::RenderWindow& window) {
    auto& font = AssetManager::get().getFont("MainFont");
    UIKit::drawBackground(window);
    UIKit::drawTitle(window, font, {90.f, 44.f}, "Full Time", 40);

    // --- Scoreboard ---
    UIKit::drawPanel(window, {160.f, 150.f, 960.f, 150.f}, false);
    auto crest = [&](const std::string& name, float cx, float cy, float box) {
        sf::Texture& tex = AssetManager::get().getTexture(name, m_isNat);
        sf::Vector2u ts = tex.getSize();
        if (ts.x > 0 && ts.y > 0) {
            float sc = box / (float)std::max(ts.x, ts.y);
            sf::Sprite s(tex); s.setScale(sc, sc);
            s.setPosition(cx - ts.x * sc * 0.5f, cy - ts.y * sc * 0.5f);
            window.draw(s);
        }
    };
    bool homeWin = m_hg > m_ag, awayWin = m_ag > m_hg;
    crest(m_hName, 320.f, 208.f, 64.f);
    crest(m_aName, 960.f, 208.f, 64.f);
    UIKit::drawTextCenteredY(window, font, 320.f - UIKit::crispText(font, m_hName, 20).getGlobalBounds().width * 0.5f,
                             262.f, m_hName, 20, homeWin ? UITheme::TextWhite : UITheme::TextDim, 1.0f, homeWin);
    UIKit::drawTextCenteredY(window, font, 960.f - UIKit::crispText(font, m_aName, 20).getGlobalBounds().width * 0.5f,
                             262.f, m_aName, 20, awayWin ? UITheme::TextWhite : UITheme::TextDim, 1.0f, awayWin);
    // Big scoreline: both numbers symmetric about the centre, the dash exactly centred between
    // them (horizontally and vertically).
    const float cy = 222.f, halfGap = 36.f;
    std::string hs = std::to_string(m_hg), as = std::to_string(m_ag);
    float hw = UIKit::crispText(font, hs, 56).getGlobalBounds().width;
    float dw = UIKit::crispText(font, "-", 40).getGlobalBounds().width;
    UIKit::drawTextCenteredY(window, font, 640.f - halfGap - hw, cy, hs, 56, UITheme::TextWhite, 1.0f, true);
    UIKit::drawTextCenteredY(window, font, 640.f + halfGap, cy, as, 56, UITheme::TextWhite, 1.0f, true);
    UIKit::drawTextCenteredY(window, font, 640.f - dw * 0.5f, cy, "-", 40, UITheme::AccentDim, 1.0f, true);
    if (!m_penaltyNote.empty()) {
        float w = UIKit::crispText(font, m_penaltyNote, 15).getGlobalBounds().width;
        UIKit::drawText(window, font, {640.f - w * 0.5f, 276.f}, m_penaltyNote, 15, UITheme::Highlight, 1.0f, true);
    }

    // --- Team comparison ---
    // A centred, split bar: left half = home (blue fill + grey remainder), right half = away
    // (red fill + grey). Fill is each side's share of the total; 0-0 leaves the whole bar grey.
    // The stat name sits UNDERNEATH the bar; the two numbers keep their left/right positions.
    const sf::Color awayC(230, 90, 90), greyC(255, 255, 255, 34);
    float y = 322.f;
    auto compRow = [&](const std::string& label, int h, int a) {
        float mid = y + 10.f;
        UIKit::drawTextCenteredY(window, font, 468.f - UIKit::crispText(font, std::to_string(h), 22).getGlobalBounds().width,
                                 mid, std::to_string(h), 22, h >= a ? UITheme::TextWhite : UITheme::TextDim, 1.0f, true);
        UIKit::drawTextCenteredY(window, font, 812.f, mid, std::to_string(a), 22,
                                 a >= h ? UITheme::TextWhite : UITheme::TextDim, 1.0f, true);

        const float x0 = 496.f, x1 = 784.f, half = (x1 - x0) * 0.5f, bh = 9.f, by = mid - bh * 0.5f;
        window.draw(UIKit::roundedRect({x0, by}, {x1 - x0, bh}, 4.f, greyC)); // grey background
        int tot = h + a;
        if (tot > 0) {
            float hw = half * (float)h / tot, aw = half * (float)a / tot;
            if (hw > 0.5f) window.draw(UIKit::roundedRect({640.f - hw, by}, {hw, bh}, 4.f, UITheme::Accent));
            if (aw > 0.5f) window.draw(UIKit::roundedRect({640.f, by}, {aw, bh}, 4.f, awayC));
        }
        sf::RectangleShape divi({2.f, bh + 6.f}); divi.setPosition(639.f, by - 3.f);
        divi.setFillColor(sf::Color(10, 14, 24)); window.draw(divi);

        float lw = UIKit::crispText(font, label, 14).getGlobalBounds().width;
        UIKit::drawText(window, font, {640.f - lw * 0.5f, y + 22.f}, label, 14, UITheme::Accent, 1.4f, true);
        y += 48.f;
    };
    compRow("SHOTS", m_hs, m_as);
    compRow("YELLOW CARDS", m_hy, m_ay);
    compRow("RED CARDS", m_hr, m_ar);

    // --- Player rating card ---
    UIKit::drawPanel(window, {380.f, 470.f, 520.f, 120.f});
    if (m_benched) {
        UIKit::drawText(window, font, {410.f, 512.f}, "Benched - coach trust too low", 20, UITheme::TextDim, 1.0f);
    } else {
        sf::Color rc = m_rating >= 7.f ? sf::Color(90, 210, 120)
                     : m_rating >= 6.f ? UITheme::Accent : sf::Color(230, 120, 120);
        char buf[16]; std::snprintf(buf, sizeof(buf), "%.1f", m_rating);
        UIKit::drawText(window, font, {410.f, 486.f}, buf, 46, rc, 1.0f, true);
        UIKit::drawText(window, font, {410.f, 546.f}, "MATCH RATING  / 10", 13, UITheme::TextDim, 1.6f);

        float rx = 560.f;
        UIKit::drawIcon(window, "chart", {rx + 9.f, 508.f}, 9.f, UITheme::Accent);
        UIKit::drawText(window, font, {rx + 26.f, 496.f}, "XP  +" + std::to_string(m_xpGain), 20, UITheme::TextWhite, 1.0f, true);
        if (m_trustGain != 0) {
            UIKit::drawIcon(window, "smiley", {rx + 9.f, 540.f}, 9.f, m_trustGain > 0 ? sf::Color(90, 210, 120) : sf::Color(230, 120, 120));
            std::string ts = (m_trustGain > 0 ? "Trust  +" : "Trust  ") + std::to_string(m_trustGain);
            UIKit::drawText(window, font, {rx + 26.f, 528.f}, ts, 20, m_trustGain > 0 ? sf::Color(90, 210, 120) : sf::Color(230, 120, 120), 1.0f, true);
        }
    }

    UIKit::BtnState bs = m_pressed && m_hover ? UIKit::BtnState::Pressed
                       : m_hover ? UIKit::BtnState::Hover : UIKit::BtnState::Normal;
    UIKit::drawButton(window, font, m_continueBtn, "Continue", bs);
}
