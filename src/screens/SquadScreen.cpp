#include "SquadScreen.h"
#include "UIKit.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "CareerManager.h"
#include "Player.h"
#include "AssetManager.h"
#include "SeasonEndScreen.h"
#include "UITheme.h"
#include <algorithm>

SquadScreen::SquadScreen() {
}

void SquadScreen::init() {
    m_rows.clear();
    m_scroll = 0.f;

    Player* p = m_gameManager->getPlayer();
    if (!p || !p->currentClub) return;
    m_title = p->currentClub->name + " - Squad";

    auto roster = p->currentClub->roster; // copy of pointers
    std::sort(roster.begin(), roster.end(), [](AIPlayer* a, AIPlayer* b) {
        if (a->position != b->position) return (int)a->position < (int)b->position;
        return a->overall > b->overall;
    });

    m_rows.push_back({p->name + " (You)", p->nationality, p->position, p->overall(), p->goals, p->assists, true});
    for (auto aip : roster)
        m_rows.push_back({aip->name, aip->nationality, aip->position, aip->overall, aip->goals, aip->assists, false});

    float visible = m_bandBot - m_bandTop;
    m_maxScroll = std::max(0.f, m_rows.size() * m_rowH - visible);

    m_backBtn = sf::FloatRect(90.f, 634.f, 200.f, 48.f);
}

void SquadScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        m_backHover = m_backBtn.contains(m);
    }
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        m_backPressed = m_backBtn.contains(m);
    }
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        if (m_backPressed && m_backBtn.contains(m)) {
            if (m_gameManager->getPlayer()->weeksPlayed >= m_gameManager->getCareerManager()->getSeasonLength())
                m_gameManager->changeScreen(std::make_shared<SeasonEndScreen>());
            else
                m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
        }
        m_backPressed = false;
    }
    if (event.type == sf::Event::MouseWheelScrolled) {
        m_scroll = std::clamp(m_scroll - event.mouseWheelScroll.delta * 34.f, 0.f, m_maxScroll);
    }
}

void SquadScreen::update(sf::Time) {}

static void posBadge(sf::RenderWindow& window, sf::Font& font, sf::Vector2f pos, PlayerPosition p) {
    std::string s = "GK"; sf::Color c(230, 200, 70);
    if (p == PlayerPosition::Defender)   { s = "DEF"; c = sf::Color(80, 150, 240); }
    else if (p == PlayerPosition::Midfielder) { s = "MID"; c = sf::Color(90, 210, 120); }
    else if (p == PlayerPosition::Forward)    { s = "FWD"; c = sf::Color(235, 100, 100); }
    window.draw(UIKit::roundedRect(pos, {54.f, 24.f}, 5.f, sf::Color(c.r, c.g, c.b, 55), 1.5f, c));
    UIKit::drawText(window, font, {pos.x + 10.f, pos.y + 3.f}, s, 14, c, 1.0f, true);
}

void SquadScreen::draw(sf::RenderWindow& window) {
    auto& font = AssetManager::get().getFont("MainFont");

    UIKit::drawBackground(window);
    UIKit::drawTitle(window, font, {90.f, 40.f}, m_title.empty() ? "Squad" : m_title, 34);

    // Column headers.
    const float cName = 116.f, cPos = 640.f, cNat = 740.f, cOvr = 850.f, cG = 960.f, cA = 1050.f;
    float hy = 176.f;
    UIKit::drawText(window, font, {cName, hy}, "NAME", 15, UITheme::Accent, 1.6f, true);
    UIKit::drawText(window, font, {cPos, hy}, "POS", 15, UITheme::Accent, 1.6f, true);
    UIKit::drawText(window, font, {cNat, hy}, "NAT", 15, UITheme::Accent, 1.6f, true);
    UIKit::drawText(window, font, {cOvr, hy}, "OVR", 15, UITheme::Accent, 1.6f, true);
    UIKit::drawText(window, font, {cG, hy}, "G", 15, UITheme::Accent, 1.6f, true);
    UIKit::drawText(window, font, {cA, hy}, "A", 15, UITheme::Accent, 1.6f, true);

    // Clip the scrolling table to its band, letterbox-aware (works in fullscreen too).
    sf::View base = window.getView();
    sf::FloatRect vp = base.getViewport();
    float bandH = m_bandBot - m_bandTop;
    sf::View clip = base;
    clip.reset(sf::FloatRect(0.f, m_bandTop, 1280.f, bandH));
    clip.setViewport(sf::FloatRect(vp.left, vp.top + vp.height * (m_bandTop / 720.f),
                                   vp.width, vp.height * (bandH / 720.f)));
    window.setView(clip);

    for (size_t i = 0; i < m_rows.size(); ++i) {
        float y = m_bandTop + i * m_rowH - m_scroll;
        if (y + m_rowH < m_bandTop || y > m_bandBot) continue;
        const Row& r = m_rows[i];

        sf::Color bg = r.isMe ? sf::Color(UITheme::Accent.r, UITheme::Accent.g, UITheme::Accent.b, 40)
                     : (i % 2 ? sf::Color(255, 255, 255, 10) : sf::Color(255, 255, 255, 20));
        window.draw(UIKit::roundedRect({90.f, y + 2.f}, {1090.f, m_rowH - 6.f}, 6.f, bg));
        if (r.isMe) {
            sf::RectangleShape edge({4.f, m_rowH - 10.f});
            edge.setPosition(90.f, y + 4.f); edge.setFillColor(UITheme::Accent);
            window.draw(edge);
        }

        sf::Color tc = r.isMe ? UITheme::TextWhite : UITheme::TextDim;
        UIKit::drawText(window, font, {cName, y + 12.f}, r.name, 18, tc, 1.0f, r.isMe);
        posBadge(window, font, {cPos, y + 9.f}, r.pos);
        UIKit::drawText(window, font, {cNat, y + 12.f}, r.nat.substr(0, 3), 18, tc, 1.0f);
        UIKit::drawText(window, font, {cOvr, y + 12.f}, std::to_string(r.ovr), 18,
                        r.isMe ? UITheme::Accent : UITheme::TextWhite, 1.0f, true);
        UIKit::drawText(window, font, {cG, y + 12.f}, std::to_string(r.goals), 18, tc, 1.0f);
        UIKit::drawText(window, font, {cA, y + 12.f}, std::to_string(r.assists), 18, tc, 1.0f);
    }

    window.setView(base);

    // Scrollbar hint.
    if (m_maxScroll > 0.f) {
        float trackH = m_bandBot - m_bandTop;
        float thumbH = std::max(30.f, trackH * (trackH / (trackH + m_maxScroll)));
        float t = m_scroll / m_maxScroll;
        window.draw(UIKit::roundedRect({1196.f, m_bandTop + t * (trackH - thumbH)}, {6.f, thumbH}, 3.f, UITheme::AccentDim));
    }

    UIKit::BtnState bs = m_backPressed && m_backHover ? UIKit::BtnState::Pressed
                       : m_backHover ? UIKit::BtnState::Hover : UIKit::BtnState::Normal;
    UIKit::drawButton(window, font, m_backBtn, "Back", bs);
}
