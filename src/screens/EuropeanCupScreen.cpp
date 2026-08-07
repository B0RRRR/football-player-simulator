#include "UITheme.h"
#include "UIKit.h"
#include "EuropeanCupScreen.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "CareerManager.h"
#include "Player.h"
#include "AssetManager.h"
#include "SeasonEndScreen.h"
#include <sstream>
#include <algorithm>

EuropeanCupScreen::EuropeanCupScreen() : m_currentView(0), m_selectedYear(0), m_maxYear(0) {
}

void EuropeanCupScreen::init() {
    m_maxYear = m_gameManager->getCareerManager()->getYear();
    m_selectedYear = m_maxYear;

    m_tabs.clear();
    m_tabs.push_back({sf::FloatRect(60.f,  150.f, 230.f, 44.f), "Champions League", "CL"});
    m_tabs.push_back({sf::FloatRect(300.f, 150.f, 210.f, 44.f), "Europa League",    "EL"});
    m_tabs.push_back({sf::FloatRect(520.f, 150.f, 220.f, 44.f), "Int. Tournament",  "INT"});
    m_backBtn = sf::FloatRect(1030.f, 636.f, 200.f, 48.f);
    m_prevBtn = sf::FloatRect(1096.f, 44.f, 42.f, 40.f);
    m_nextBtn = sf::FloatRect(1216.f, 44.f, 42.f, 40.f);

    updateBracketVisuals();
}

void EuropeanCupScreen::updateBracketVisuals() {
    m_rounds.clear();
    m_isNat = (m_currentView == 2);

    Database& db = m_gameManager->getDatabase();
    Tournament t;
    bool isHistory = (m_selectedYear != m_maxYear);

    if (m_currentView == 0) {
        t = (isHistory && db.getChampionsLeagueHistory().count(m_selectedYear))
            ? db.getChampionsLeagueHistory().at(m_selectedYear) : db.getChampionsLeague();
    } else if (m_currentView == 1) {
        t = (isHistory && db.getEuropaLeagueHistory().count(m_selectedYear))
            ? db.getEuropaLeagueHistory().at(m_selectedYear) : db.getEuropaLeague();
    } else {
        if (m_selectedYear % 2 == 0)
            t = (isHistory && db.getEuroCupHistory().count(m_selectedYear))
                ? db.getEuroCupHistory().at(m_selectedYear) : db.getEuroCup();
        else
            t = (isHistory && db.getWorldCupHistory().count(m_selectedYear))
                ? db.getWorldCupHistory().at(m_selectedYear) : db.getWorldCup();
    }

    Player* p = m_gameManager->getPlayer();
    bool found = false;

    for (size_t i = 0; i < t.rounds.size(); ++i) {
        BRound br; br.name = t.rounds[i].name;
        for (const auto& m : t.rounds[i].matches) {
            BMatch bm;
            bm.home = m.home ? m.home->name : "TBD";
            bm.away = m.away ? m.away->name : "TBD";
            bm.winner = (m.winner && m.winner == m.home) ? 1 : (m.winner && m.winner == m.away) ? 2 : 0;
            bm.me = false;
            if (m_currentView < 2) {
                if (m.home && p && p->currentClub && m.home->name == p->currentClub->name) bm.me = true;
                if (m.away && p && p->currentClub && m.away->name == p->currentClub->name) bm.me = true;
            } else {
                if (m.home && p && m.home->name == p->nationality && p->isCalledUp) bm.me = true;
                if (m.away && p && m.away->name == p->nationality && p->isCalledUp) bm.me = true;
            }
            if (bm.me) found = true;

            // Two-legged ties show BOTH legs stacked (leg1 on the home row, leg2 on the away
            // row, from that leg's host perspective). Single matches (internationals, finals)
            // show one "h:a" line.
            auto pens = [&]() { return " (" + std::to_string(m.homePenalties) + "-" + std::to_string(m.awayPenalties) + "p)"; };
            if (m.leg2Played) {
                bm.s1 = std::to_string(m.homeGoalsLeg1) + ":" + std::to_string(m.awayGoalsLeg1);
                bm.s2 = std::to_string(m.homeGoalsLeg2) + ":" + std::to_string(m.awayGoalsLeg2);
                int a1 = m.homeGoalsLeg1 + m.awayGoalsLeg2, a2 = m.awayGoalsLeg1 + m.homeGoalsLeg2;
                if (a1 == a2) bm.s2 += pens();
            } else if (m.leg1Played) {
                bm.s1 = std::to_string(m.homeGoalsLeg1) + ":" + std::to_string(m.awayGoalsLeg1);
                if (m.isFinal && m.homeGoalsLeg1 == m.awayGoalsLeg1) bm.s1 += pens();
                bm.s2 = "";
            } else {
                bm.s1 = "vs"; bm.s2 = "";
            }
            br.matches.push_back(bm);
        }
        m_rounds.push_back(br);
    }

    m_viewName = m_currentView == 0 ? "Champions League"
               : m_currentView == 1 ? "Europa League"
               : (m_selectedYear % 2 == 0 ? "Euro Cup" : "World Cup");
    if (t.rounds.empty()) m_statusStr = "Tournament has not been generated yet.";
    else if (found && !isHistory) m_statusStr = "You are participating in this tournament.";
    else if (isHistory) m_statusStr = "Historical view.";
    else m_statusStr = "You are not participating in this tournament.";
    m_winnerStr = (t.isFinished && t.winner) ? ("Winner: " + t.winner->name) : "";
}

void EuropeanCupScreen::dispatch(const std::string& action) {
    if (action == "CL")  { m_currentView = 0; updateBracketVisuals(); }
    else if (action == "EL")  { m_currentView = 1; updateBracketVisuals(); }
    else if (action == "INT") { m_currentView = 2; updateBracketVisuals(); }
    else if (action == "PREV") { if (m_selectedYear > 2024) { m_selectedYear--; updateBracketVisuals(); } }
    else if (action == "NEXT") { if (m_selectedYear < m_maxYear) { m_selectedYear++; updateBracketVisuals(); } }
    else if (action == "BACK") {
        if (m_gameManager->getPlayer()->weeksPlayed >= m_gameManager->getCareerManager()->getSeasonLength())
            m_gameManager->changeScreen(std::make_shared<SeasonEndScreen>());
        else
            m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
    }
}

void EuropeanCupScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    auto actionAt = [&](sf::Vector2f m) -> std::string {
        if (m_backBtn.contains(m)) return "BACK";
        if (m_prevBtn.contains(m)) return "PREV";
        if (m_nextBtn.contains(m)) return "NEXT";
        for (auto& t : m_tabs) if (t.bounds.contains(m)) return t.action;
        return "";
    };
    if (event.type == sf::Event::MouseMoved)
        m_hoverAction = actionAt(window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y}));
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        m_pressedAction = actionAt(window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y}));
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        std::string rel = actionAt(window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y}));
        if (!rel.empty() && rel == m_pressedAction) dispatch(rel);
        m_pressedAction = "";
    }
}

void EuropeanCupScreen::update(sf::Time) {}

void EuropeanCupScreen::draw(sf::RenderWindow& window) {
    auto& font = AssetManager::get().getFont("MainFont");

    UIKit::drawBackground(window);
    UIKit::drawTitle(window, font, {60.f, 40.f}, "Tournaments", 36);

    auto stateOf = [&](const std::string& a) {
        if (a == m_pressedAction && a == m_hoverAction) return UIKit::BtnState::Pressed;
        if (a == m_hoverAction) return UIKit::BtnState::Hover;
        return UIKit::BtnState::Normal;
    };

    // Status strap.
    UIKit::drawText(window, font, {62.f, 100.f}, m_viewName + "  " + std::to_string(m_selectedYear), 16, UITheme::Accent, 1.4f, true);
    UIKit::drawText(window, font, {62.f, 124.f}, m_statusStr, 15, UITheme::TextDim, 1.0f);
    if (!m_winnerStr.empty())
        UIKit::drawText(window, font, {430.f, 124.f}, m_winnerStr, 15, UITheme::Highlight, 1.0f, true);

    // Year stepper.
    UIKit::drawText(window, font, {1096.f, 16.f}, "SEASON", 13, UITheme::Accent, 2.0f, true);
    UIKit::drawButton(window, font, m_prevBtn, "<", stateOf("PREV"), false);
    UIKit::drawButton(window, font, m_nextBtn, ">", stateOf("NEXT"), false);
    UIKit::drawText(window, font, {1156.f, 52.f}, std::to_string(m_selectedYear), 20, UITheme::TextWhite, 1.0f, true);

    // Tabs (current view highlighted). No chevron - these are toggles, not "go" actions.
    const char* cur = m_currentView == 0 ? "CL" : m_currentView == 1 ? "EL" : "INT";
    for (auto& t : m_tabs) {
        UIKit::BtnState st = stateOf(t.action);
        if (t.action == cur && st == UIKit::BtnState::Normal) st = UIKit::BtnState::Hover;
        UIKit::drawButton(window, font, t.bounds, t.label, st, false);
    }

    // Bracket: match cards with crests, linked by connector lines.
    int R = (int)m_rounds.size();
    if (R > 0) {
        const float left = 60.f, right = 1256.f, ytop = 232.f, ybot = 690.f;
        float colW = (right - left) / R;
        float cardW = std::min(colW - 22.f, 250.f);
        int M = std::max(1, (int)m_rounds[0].matches.size());
        float slotH = (ybot - ytop) / M;
        float cardH = std::clamp(slotH * 0.82f, 20.f, 50.f); // never taller than its slot
        auto cyOf = [&](int r, int j) { return ytop + slotH * (float)(1 << r) * (j + 0.5f); };
        auto colX = [&](int r) { return left + r * colW; };

        // Connectors (behind the cards).
        for (int r = 0; r + 1 < R; ++r)
            for (int j = 0; j < (int)m_rounds[r].matches.size(); ++j) {
                int pj = j / 2;
                if (pj >= (int)m_rounds[r + 1].matches.size()) continue;
                float ax = colX(r) + cardW, ay = cyOf(r, j), bx = colX(r + 1), by = cyOf(r + 1, pj);
                float mx = (ax + bx) * 0.5f;
                sf::VertexArray v(sf::LineStrip, 4);
                sf::Color lc = UITheme::AccentDim;
                v[0] = {{ax, ay}, lc}; v[1] = {{mx, ay}, lc}; v[2] = {{mx, by}, lc}; v[3] = {{bx, by}, lc};
                window.draw(v);
            }

        auto trunc = [&](const std::string& s, float w) {
            int budget = std::max(3, (int)(w / (13.f * 0.56f)));
            if ((int)UITheme::utf8Length(s) <= budget) return s;
            std::string out; int n = 0;
            for (size_t i = 0; i < s.size();) {
                unsigned char c = s[i]; int len = (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
                if (n >= budget - 1) { out += "."; break; }
                out += s.substr(i, len); i += len; ++n;
            }
            return out;
        };

        for (int r = 0; r < R; ++r) {
            float cx = colX(r);
            UIKit::drawText(window, font, {cx, ytop - 30.f}, UIKit::upper(m_rounds[r].name), 15, UITheme::Accent, 1.3f, true);
            for (int j = 0; j < (int)m_rounds[r].matches.size(); ++j) {
                const BMatch& m = m_rounds[r].matches[j];
                float cy = cyOf(r, j), cardY = cy - cardH * 0.5f;
                window.draw(UIKit::roundedRect({cx, cardY}, {cardW, cardH}, 6.f, UITheme::PanelDark,
                                               m.me ? 2.f : 1.f, m.me ? UITheme::Accent : sf::Color(255, 255, 255, 20)));
                sf::RectangleShape dv({cardW - 12.f, 1.f}); dv.setPosition(cx + 6.f, cardY + cardH * 0.5f);
                dv.setFillColor(sf::Color(255, 255, 255, 25)); window.draw(dv);

                float logoBox = std::max(8.f, cardH * 0.5f - 6.f), nameW = cardW - logoBox - 78.f;
                auto side = [&](const std::string& name, float rowCentre, bool win) {
                    sf::Texture& tex = AssetManager::get().getTexture(name, m_isNat);
                    sf::Vector2u ts = tex.getSize();
                    if (name != "TBD" && ts.x > 0 && ts.y > 0) {
                        float sc = logoBox / (float)std::max(ts.x, ts.y);
                        sf::Sprite lg(tex); lg.setScale(sc, sc);
                        lg.setPosition(cx + 8.f + (logoBox - ts.x * sc) * 0.5f, rowCentre - ts.y * sc * 0.5f);
                        window.draw(lg);
                    }
                    sf::Color tc = m.me ? UITheme::Highlight : (win ? UITheme::TextWhite : UITheme::TextDim);
                    UIKit::drawTextCenteredY(window, font, cx + 8.f + logoBox + 8.f, rowCentre,
                                             trunc(name, nameW), 13, tc, 1.0f, win);
                };
                float topC = cardY + cardH * 0.25f, botC = cardY + cardH * 0.75f;
                side(m.home, topC, m.winner == 1);
                side(m.away, botC, m.winner == 2);

                // Score: single line centred, or two legs stacked on the two rows.
                if (m.s2.empty()) {
                    float sw = UIKit::crispText(font, m.s1, 13).getGlobalBounds().width;
                    UIKit::drawTextCenteredY(window, font, cx + cardW - 8.f - sw, cy, m.s1, 13, UITheme::Accent, 1.0f, true);
                } else {
                    float w1 = UIKit::crispText(font, m.s1, 12).getGlobalBounds().width;
                    float w2 = UIKit::crispText(font, m.s2, 12).getGlobalBounds().width;
                    UIKit::drawTextCenteredY(window, font, cx + cardW - 8.f - w1, topC, m.s1, 12, UITheme::Accent, 1.0f, true);
                    UIKit::drawTextCenteredY(window, font, cx + cardW - 8.f - w2, botC, m.s2, 12, UITheme::Accent, 1.0f, true);
                }
            }
        }
    }

    UIKit::drawButton(window, font, m_backBtn, "Back", stateOf("BACK"));
}
