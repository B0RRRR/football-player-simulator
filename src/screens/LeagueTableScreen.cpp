#include "UITheme.h"
#include "UIKit.h"
#include "LeagueTableScreen.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "CareerManager.h"
#include "Player.h"
#include "AssetManager.h"
#include "SeasonEndScreen.h"
#include <algorithm>
#include <set>

LeagueTableScreen::LeagueTableScreen() {
}

void LeagueTableScreen::init() {
    m_viewedYear = m_gameManager->getCareerManager()->getYear();
    m_backBtn = sf::FloatRect(1030.f, 636.f, 200.f, 48.f); // bottom-right
    m_prevBtn = sf::FloatRect(1096.f, 44.f, 42.f, 40.f);
    m_nextBtn = sf::FloatRect(1216.f, 44.f, 42.f, 40.f);
}

void LeagueTableScreen::dispatch(const std::string& action) {
    if (action == "Back") {
        if (m_gameManager->getPlayer()->weeksPlayed >= m_gameManager->getCareerManager()->getSeasonLength())
            m_gameManager->changeScreen(std::make_shared<SeasonEndScreen>());
        else
            m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
    } else if (action == "Prev") {
        m_viewedYear--;
    } else if (action == "Next") {
        m_viewedYear++;
    } else if (action.rfind("LG_", 0) == 0) {
        m_viewedLeagueName = action.substr(3);
    }
}

void LeagueTableScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    auto actionAt = [&](sf::Vector2f m) -> std::string {
        if (m_backBtn.contains(m)) return "Back";
        if (m_prevBtn.contains(m)) return "Prev";
        if (m_nextBtn.contains(m)) return "Next";
        for (auto& t : m_leagueTabs) if (t.bounds.contains(m)) return "LG_" + t.name;
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

void LeagueTableScreen::update(sf::Time) {
    m_standings.clear();
    m_leagueTabs.clear();

    Player* p = m_gameManager->getPlayer();
    if (!p || !p->currentClub) return;

    int currentYear = m_gameManager->getCareerManager()->getYear();
    m_viewedYear = std::clamp(m_viewedYear, 2024, currentYear);

    const std::vector<League>* src = nullptr;
    if (m_viewedYear == currentYear) src = &m_gameManager->getDatabase().getLeagues();
    else {
        auto& hist = m_gameManager->getDatabase().getLeagueHistory();
        if (hist.count(m_viewedYear)) src = &hist.at(m_viewedYear);
    }
    if (!src || src->empty()) return;

    if (m_viewedLeagueName.empty()) {
        for (const auto& l : *src)
            for (const auto& c : l.clubs)
                if (c.name == p->currentClub->name) { m_viewedLeagueName = l.name; break; }
        if (m_viewedLeagueName.empty()) m_viewedLeagueName = (*src)[0].name;
    }
    const League* league = nullptr;
    for (const auto& l : *src) if (l.name == m_viewedLeagueName) { league = &l; break; }
    if (!league) league = &(*src)[0];
    m_viewedLeagueName = league->name;

    // League selector tabs (right column).
    float ty = 150.f;
    for (const auto& l : *src) {
        m_leagueTabs.push_back({sf::FloatRect(1000.f, ty, 250.f, 34.f), l.name});
        ty += 40.f;
    }

    m_titleStr = league->name + "  " + std::to_string(m_viewedYear) + "/" + std::to_string((m_viewedYear + 1) % 100);

    std::vector<Club> clubs = league->clubs;
    std::sort(clubs.begin(), clubs.end(), [](const Club& a, const Club& b) {
        if (a.points != b.points) return a.points > b.points;
        int ga = a.goalsFor - a.goalsAgainst, gb = b.goalsFor - b.goalsAgainst;
        if (ga != gb) return ga > gb;
        return a.goalsFor > b.goalsFor;
    });
    bool isNat = (league->name == "National Teams");

    // Qualification / promotion / relegation zones, keyed off the league's tier (top divisions
    // send 1-3 to CL, 4-6 to EL, bottom 3 down; second divisions send the top 3 up).
    static const std::set<std::string> topTier = {
        "Premier League", "La Liga", "Serie A", "Bundesliga", "Ligue 1"};
    static const std::set<std::string> secondTier = {
        "Championship", "Segunda Division", "Serie B", "2. Bundesliga", "Ligue 2"};
    m_topTier = topTier.count(league->name) > 0;
    m_secondTier = secondTier.count(league->name) > 0;
    int n = (int)clubs.size();

    for (size_t i = 0; i < clubs.size(); ++i) {
        const auto& c = clubs[i];
        int pos = (int)i + 1, zone = 0;
        if (m_topTier) {
            if (pos <= 3) zone = 1;
            else if (pos <= 6) zone = 2;
            else if (pos > n - 3) zone = 3;
        } else if (m_secondTier) {
            if (pos <= 3) zone = 1;
        }
        m_standings.push_back({pos, c.name, isNat, c.points, c.wins, c.draws, c.losses,
                               c.goalsFor, c.goalsAgainst, c.goalsFor - c.goalsAgainst,
                               c.name == p->currentClub->name, zone});
    }
}

void LeagueTableScreen::draw(sf::RenderWindow& window) {
    auto& font = AssetManager::get().getFont("MainFont");

    UIKit::drawBackground(window);
    UIKit::drawTitle(window, font, {60.f, 40.f}, m_titleStr.empty() ? "League Table" : m_titleStr, 30);

    auto stateOf = [&](const std::string& a) {
        if (a == m_pressedAction && a == m_hoverAction) return UIKit::BtnState::Pressed;
        if (a == m_hoverAction) return UIKit::BtnState::Hover;
        return UIKit::BtnState::Normal;
    };

    // Year stepper (top-right).
    UIKit::drawText(window, font, {1096.f, 16.f}, "SEASON", 13, UITheme::Accent, 2.0f, true);
    UIKit::drawButton(window, font, m_prevBtn, "<", stateOf("Prev"), false);
    UIKit::drawButton(window, font, m_nextBtn, ">", stateOf("Next"), false);
    UIKit::drawText(window, font, {1156.f, 52.f}, std::to_string(m_viewedYear), 20, UITheme::TextWhite, 1.0f, true);

    // League tabs.
    for (auto& t : m_leagueTabs) {
        UIKit::BtnState st = stateOf("LG_" + t.name);
        if (t.name == m_viewedLeagueName && st == UIKit::BtnState::Normal) st = UIKit::BtnState::Hover;
        UIKit::drawButton(window, font, t.bounds, t.name, st, false);
    }

    // Column headers.
    const float cPos = 74.f, cLogo = 120.f, cClub = 150.f, cPts = 640.f,
                cW = 700.f, cD = 748.f, cL = 796.f, cGF = 848.f, cGA = 900.f, cGD = 952.f;
    float hy = 118.f;
    UIKit::drawText(window, font, {cPos, hy}, "#", 14, UITheme::Accent, 1.4f, true);
    UIKit::drawText(window, font, {cClub, hy}, "CLUB", 14, UITheme::Accent, 1.4f, true);
    UIKit::drawText(window, font, {cPts, hy}, "PTS", 14, UITheme::Accent, 1.4f, true);
    UIKit::drawText(window, font, {cW, hy}, "W", 14, UITheme::Accent, 1.4f, true);
    UIKit::drawText(window, font, {cD, hy}, "D", 14, UITheme::Accent, 1.4f, true);
    UIKit::drawText(window, font, {cL, hy}, "L", 14, UITheme::Accent, 1.4f, true);
    UIKit::drawText(window, font, {cGF, hy}, "GF", 14, UITheme::Accent, 1.4f, true);
    UIKit::drawText(window, font, {cGA, hy}, "GA", 14, UITheme::Accent, 1.4f, true);
    UIKit::drawText(window, font, {cGD, hy}, "GD", 14, UITheme::Accent, 1.4f, true);

    // Rows.
    auto zoneColor = [](int z) {
        return z == 1 ? sf::Color(90, 210, 120)     // CL / promotion
             : z == 2 ? sf::Color(240, 170, 70)     // EL
             : z == 3 ? sf::Color(230, 90, 90)      // relegation
                      : sf::Color::Transparent;
    };
    // Row height shrinks a touch when a division is large (e.g. 22 clubs) so the table always
    // ends above the legend/back area instead of overlapping it.
    int nrows = (int)m_standings.size();
    float y = 142.f;
    const float rh = std::min(24.5f, (628.f - 142.f) / std::max(1, nrows));
    float box = std::min(18.f, rh - 6.f);
    for (const auto& s : m_standings) {
        float rc = y + (rh - 3.f) * 0.5f; // row centre-line
        sf::Color bg = s.isMe ? sf::Color(UITheme::Accent.r, UITheme::Accent.g, UITheme::Accent.b, 46)
                     : (s.pos % 2 ? sf::Color(255, 255, 255, 8) : sf::Color(255, 255, 255, 16));
        window.draw(UIKit::roundedRect({60.f, y}, {920.f, rh - 3.f}, 4.f, bg));
        if (s.zone != 0)
            window.draw(UIKit::roundedRect({61.f, y + 2.f}, {5.f, rh - 7.f}, 2.f, zoneColor(s.zone)));
        sf::Color tc = s.isMe ? UITheme::TextWhite : UITheme::TextDim;

        UIKit::drawTextCenteredY(window, font, cPos, rc, std::to_string(s.pos), 16, tc, 1.0f, s.isMe);

        sf::Texture& tex = AssetManager::get().getTexture(s.club, s.isNat);
        sf::Vector2u ts = tex.getSize();
        if (ts.x > 0 && ts.y > 0) {
            float sc = box / (float)std::max(ts.x, ts.y);
            sf::Sprite lg(tex); lg.setScale(sc, sc);
            lg.setPosition(cLogo + (box - ts.x * sc) * 0.5f, rc - ts.y * sc * 0.5f);
            window.draw(lg);
        }
        UIKit::drawTextCenteredY(window, font, cClub, rc, s.club, 16, s.isMe ? UITheme::Accent : UITheme::TextWhite, 1.0f, s.isMe);
        UIKit::drawTextCenteredY(window, font, cPts, rc, std::to_string(s.pts), 16, UITheme::TextWhite, 1.0f, true);
        UIKit::drawTextCenteredY(window, font, cW, rc, std::to_string(s.w), 16, tc);
        UIKit::drawTextCenteredY(window, font, cD, rc, std::to_string(s.d), 16, tc);
        UIKit::drawTextCenteredY(window, font, cL, rc, std::to_string(s.l), 16, tc);
        UIKit::drawTextCenteredY(window, font, cGF, rc, std::to_string(s.gf), 16, tc);
        UIKit::drawTextCenteredY(window, font, cGA, rc, std::to_string(s.ga), 16, tc);
        UIKit::drawTextCenteredY(window, font, cGD, rc, (s.gd > 0 ? "+" : "") + std::to_string(s.gd), 16, tc);
        y += rh;
    }

    // Zone legend (bottom-left), matching the current league's tier.
    if (m_topTier || m_secondTier) {
        float lx = 60.f, ly = 642.f;
        auto chip = [&](sf::Color col, const std::string& text) {
            window.draw(UIKit::roundedRect({lx, ly + 2.f}, {12.f, 12.f}, 3.f, col));
            UIKit::drawText(window, font, {lx + 18.f, ly}, text, 14, UITheme::TextDim, 1.0f);
            lx += 20.f + UIKit::crispText(font, text, 14).getGlobalBounds().width + 26.f;
        };
        if (m_topTier) {
            chip(sf::Color(90, 210, 120), "Champions League");
            chip(sf::Color(240, 170, 70), "Europa League");
            chip(sf::Color(230, 90, 90), "Relegation");
        } else {
            chip(sf::Color(90, 210, 120), "Promotion");
        }
    }

    UIKit::drawButton(window, font, m_backBtn, "Back", stateOf("Back"));
}
