#include "UITheme.h"
#include "UIKit.h"
#include "SeasonEndScreen.h"
#include "TransferScreen.h"
#include "GameManager.h"
#include "CareerManager.h"
#include "Player.h"
#include "AssetManager.h"
#include "LeagueTableScreen.h"
#include "EuropeanCupScreen.h"
#include "SquadScreen.h"

SeasonEndScreen::SeasonEndScreen() {
}

void SeasonEndScreen::init() {
    m_buttons.clear();
    m_hoverIdx = m_pressedIdx = -1;
    struct Def { std::string label, action; bool primary; };
    std::vector<Def> defs = {
        {"View League Table",      "LEAGUE", false},
        {"View European Cups",     "CUPS",   false},
        {"View Squad Stats",       "SQUAD",  false},
        {"Proceed to Pre-Season",  "NEXT",   true},
    };
    const float x = 620.f, w = 460.f, h = 58.f, gap = 74.f, y0 = 200.f;
    for (size_t i = 0; i < defs.size(); ++i)
        m_buttons.push_back({sf::FloatRect(x, y0 + i * gap, w, h), defs[i].label, defs[i].action, defs[i].primary});
}

void SeasonEndScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        m_hoverIdx = -1;
        for (size_t i = 0; i < m_buttons.size(); ++i) if (m_buttons[i].bounds.contains(m)) m_hoverIdx = (int)i;
    }
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        m_pressedIdx = -1;
        for (size_t i = 0; i < m_buttons.size(); ++i) if (m_buttons[i].bounds.contains(m)) m_pressedIdx = (int)i;
    }
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        int rel = -1;
        for (size_t i = 0; i < m_buttons.size(); ++i) if (m_buttons[i].bounds.contains(m)) rel = (int)i;
        if (rel >= 0 && rel == m_pressedIdx) {
            const std::string& a = m_buttons[rel].action;
            if (a == "NEXT") {
                m_gameManager->getCareerManager()->endSeason();
                m_gameManager->changeScreen(std::make_shared<TransferScreen>());
            } else if (a == "LEAGUE") m_gameManager->changeScreen(std::make_shared<LeagueTableScreen>());
            else if (a == "CUPS")     m_gameManager->changeScreen(std::make_shared<EuropeanCupScreen>());
            else if (a == "SQUAD")    m_gameManager->changeScreen(std::make_shared<SquadScreen>());
        }
        m_pressedIdx = -1;
    }
}

void SeasonEndScreen::update(sf::Time) {}

void SeasonEndScreen::draw(sf::RenderWindow& window) {
    auto& font = AssetManager::get().getFont("MainFont");
    Player* p = m_gameManager->getPlayer();

    UIKit::drawBackground(window);
    UIKit::drawTitle(window, font, {90.f, 44.f}, "Season Finished", 40);

    // Player summary panel.
    UIKit::drawPanel(window, {90.f, 150.f, 470.f, 380.f});
    if (p) {
        UIKit::drawText(window, font, {118.f, 168.f}, "SEASON REVIEW", 16, UITheme::Accent, 2.0f, true);
        float y = 210.f;
        auto row = [&](const std::string& ic, sf::Color icc, const std::string& s, sf::Color tc = UITheme::TextWhite) {
            UIKit::drawIcon(window, ic, {120.f, y + 18 * 0.55f}, 9.f, icc);
            UIKit::drawText(window, font, {142.f, y}, s, 18, tc, 1.0f); y += 34.f;
        };
        UIKit::drawText(window, font, {118.f, y}, p->name, 24, UITheme::TextWhite, 1.0f, true); y += 44.f;
        row("target", UITheme::Accent, "Goals     " + std::to_string(p->goals));
        row("arrow",  UITheme::Accent, "Assists   " + std::to_string(p->assists));
        if (p->currentClub)
            row("shield", UITheme::Accent, p->currentClub->name + "  (" + std::to_string(p->currentClub->points) + " pts)");
        y += 8.f;
        UIKit::drawText(window, font, {118.f, y},
                        "The season is over. Teams have been promoted\nand relegated, and you are one year older.\nGet ready for the summer transfer window!",
                        15, UITheme::TextDim, 1.0f);
    }

    // Buttons.
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        UIKit::BtnState st = UIKit::BtnState::Normal;
        if ((int)i == m_pressedIdx)     st = UIKit::BtnState::Pressed;
        else if ((int)i == m_hoverIdx)  st = UIKit::BtnState::Hover;
        UIKit::drawButton(window, font, m_buttons[i].bounds, m_buttons[i].label, st);
    }
}
