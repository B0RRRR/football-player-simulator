#include "MyStatusScreen.h"
#include "UIKit.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "Player.h"
#include "UITheme.h"

MyStatusScreen::MyStatusScreen() {
}

void MyStatusScreen::init() {
    m_buttons.clear();
    m_hoverIdx = m_pressedIdx = -1;
    m_coachMsg = "";

    struct Def { std::string label, action; };
    std::vector<Def> defs = {
        {"Request Transfer List", "RequestTransfer"},
        {"Request Playing Time",  "RequestPlayTime"},
        {"Achievements",          "ToggleAchievements"},
        {"Back",                  "Back"},
    };
    const float x = 560.f, w = 430.f, h = 52.f, gap = 60.f, y0 = 210.f;
    for (size_t i = 0; i < defs.size(); ++i) {
        Button b;
        b.bounds = sf::FloatRect(x, y0 + i * gap, w, h);
        b.label = defs[i].label; b.action = defs[i].action;
        m_buttons.push_back(b);
    }
}

void MyStatusScreen::dispatch(const std::string& action) {
    Player* p = m_gameManager->getPlayer();
    if (!p) return;
    if (action == "Back") {
        m_gameManager->changeScreen(std::make_shared<CareerHubScreen>()); return;
    }
    if (action == "ToggleAchievements") { m_showAchievements = !m_showAchievements; return; }
    if (action == "RequestTransfer") {
        if (p->isTransferListed) {
            m_coachMsg = "Coach: You are already on the transfer list.";
        } else if (p->coachTrust > 60.0f) {
            m_coachMsg = "Coach: I'm disappointed, but I'll respect your wish.\nYou are now transfer listed. Your trust drops.";
            p->isTransferListed = true;
            p->coachTrust = std::max(0.f, p->coachTrust - 20.0f);
        } else {
            m_coachMsg = "Coach: We're not selling you right now. Get back to training!";
            p->coachTrust = std::max(0.f, p->coachTrust - 10.0f);
        }
        m_messageTimer = 6.0f;
    } else if (action == "RequestPlayTime") {
        if (p->coachTrust >= 70.0f)
            m_coachMsg = "Coach: You are already a key player for us!";
        else if (p->coachTrust > 40.0f)
            m_coachMsg = "Coach: Keep working hard and you'll get your chance.";
        else {
            m_coachMsg = "Coach: You haven't earned it. Stop complaining!";
            p->coachTrust = std::max(0.f, p->coachTrust - 5.0f);
        }
        m_messageTimer = 6.0f;
    }
}

void MyStatusScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
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
        if (rel >= 0 && rel == m_pressedIdx) dispatch(m_buttons[rel].action);
        m_pressedIdx = -1;
    }
}

void MyStatusScreen::update(sf::Time deltaTime) {
    if (m_messageTimer > 0.0f) {
        m_messageTimer -= deltaTime.asSeconds();
        if (m_messageTimer <= 0.0f) m_coachMsg = "";
    }
}

void MyStatusScreen::draw(sf::RenderWindow& window) {
    auto& font = AssetManager::get().getFont("MainFont");
    Player* p = m_gameManager->getPlayer();

    UIKit::drawBackground(window);
    UIKit::drawTitle(window, font, {90.f, 110.f}, "My Status", 44);

    // Info panel (left).
    UIKit::drawPanel(window, {90.f, 200.f, 430.f, 400.f});
    if (p) {
        if (m_showAchievements) {
            UIKit::drawText(window, font, {118.f, 220.f}, "ACHIEVEMENTS", 18, UITheme::Accent, 2.0f, true);
            float y = 258.f;
            if (p->achievements.empty()) {
                UIKit::drawText(window, font, {118.f, y}, "No achievements yet - keep playing!", 17, UITheme::TextDim, 1.0f);
            } else {
                for (const auto& a : p->achievements) {
                    UIKit::drawIcon(window, "star", {122.f, y + 8.f}, 8.f, UITheme::Highlight);
                    UIKit::drawText(window, font, {140.f, y}, a, 16, UITheme::TextWhite, 1.0f);
                    y += 28.f;
                    if (y > 580.f) break;
                }
            }
        } else {
            float y = 224.f;
            auto row = [&](const std::string& ic, sf::Color icc, const std::string& s, sf::Color tc = UITheme::TextWhite) {
                UIKit::drawIcon(window, ic, {118.f, y + 18 * 0.55f}, 9.f, icc);
                UIKit::drawText(window, font, {140.f, y}, s, 18, tc, 1.0f); y += 34.f;
            };
            row("shield", UITheme::Accent, "Club:  " + std::string(p->currentClub ? p->currentClub->name : "None"));

            // Coach trust with a bar.
            UIKit::drawIcon(window, "smiley", {118.f, y + 18 * 0.55f}, 9.f, UITheme::Accent);
            UIKit::drawText(window, font, {140.f, y}, "Coach Trust", 18, UITheme::TextWhite, 1.0f);
            UIKit::drawText(window, font, {430.f, y}, std::to_string((int)p->coachTrust) + " / 100", 16, UITheme::Accent, 1.0f, true);
            y += 30.f;
            float bx = 140.f, bw = 350.f, bh = 12.f;
            window.draw(UIKit::roundedRect({bx, y}, {bw, bh}, 6.f, UITheme::PanelPressed));
            float frac = std::clamp(p->coachTrust / 100.f, 0.f, 1.f);
            sf::Color barc = p->coachTrust < 30.f ? sf::Color(230, 90, 90)
                           : p->coachTrust < 60.f ? UITheme::Highlight : UITheme::Accent;
            if (frac > 0.02f) window.draw(UIKit::roundedRect({bx, y}, {bw * frac, bh}, 6.f, barc));
            y += 30.f;

            row("star",  UITheme::Accent, std::string("Status:  ") + (p->coachTrust < 30.f ? "Benched" : "Active"),
                p->coachTrust < 30.f ? sf::Color(230, 120, 120) : UITheme::TextWhite);
            row("chart", UITheme::Accent, "Contract:  " + std::to_string(p->contractYearsLeft) + " years left");
            row("coin",  UITheme::Highlight, "Salary:  $" + std::to_string(p->salary) + "/w");
            row("arrow", UITheme::Accent, std::string("Transfer:  ") + (p->isTransferListed ? "LISTED" : "Not for sale"),
                p->isTransferListed ? UITheme::Highlight : UITheme::TextWhite);
        }
    }

    // Buttons (right).
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        UIKit::BtnState st = UIKit::BtnState::Normal;
        if ((int)i == m_pressedIdx)     st = UIKit::BtnState::Pressed;
        else if ((int)i == m_hoverIdx)  st = UIKit::BtnState::Hover;
        UIKit::drawButton(window, font, m_buttons[i].bounds, m_buttons[i].label, st);
    }

    // Coach reply.
    if (!m_coachMsg.empty()) {
        UIKit::drawPanel(window, {560.f, 470.f, 430.f, 130.f}, true);
        UIKit::drawText(window, font, {584.f, 486.f}, m_coachMsg, 16, UITheme::Highlight, 1.0f);
    }
}
