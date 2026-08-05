#include "MenuScreen.h"
#include "SettingsScreen.h"
#include "MatchScreen.h"
#include "NewCareerScreen.h"
#include "UpgradeScreen.h"
#include "AssetManager.h"
#include "GameManager.h"
#include "UITheme.h"
#include "UIKit.h"
#include "SaveManager.h"
#include "CareerHubScreen.h"
#include <iostream>

MenuScreen::MenuScreen() {
}

void MenuScreen::init() {
    m_buttons.clear();
    m_hoverIdx = m_pressedIdx = -1;

    std::vector<std::string> labels;
    if (SaveManager::hasSaveGame("savegame.json")) labels.push_back("Continue Career");
    labels.push_back("New Career");
    labels.push_back("Settings");
    labels.push_back("Exit");

    const float x = 140.f, w = 380.f, h = 60.f, gap = 74.f, startY = 300.f;
    for (size_t i = 0; i < labels.size(); ++i) {
        Button btn;
        btn.bounds = sf::FloatRect(x, startY + i * gap, w, h);
        btn.label = labels[i];
        btn.action = labels[i];
        m_buttons.push_back(btn);
    }
}

void MenuScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        m_hoverIdx = -1;
        for (size_t i = 0; i < m_buttons.size(); ++i)
            if (m_buttons[i].bounds.contains(m)) m_hoverIdx = (int)i;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        m_pressedIdx = -1;
        for (size_t i = 0; i < m_buttons.size(); ++i)
            if (m_buttons[i].bounds.contains(m)) m_pressedIdx = (int)i;
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        int released = -1;
        for (size_t i = 0; i < m_buttons.size(); ++i)
            if (m_buttons[i].bounds.contains(m)) released = (int)i;

        // Only fire if press and release land on the same button (standard click semantics).
        if (released >= 0 && released == m_pressedIdx) {
            const std::string& action = m_buttons[released].action;
            if (action == "Exit") {
                window.close();
            } else if (action == "Continue Career") {
                if (SaveManager::loadGame("savegame.json", m_gameManager->getPlayer(),
                                          m_gameManager->getCareerManager(),
                                          &m_gameManager->getDatabase())) {
                    m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
                } else {
                    std::cout << "Failed to load savegame.\n";
                }
            } else if (action == "New Career") {
                m_gameManager->changeScreen(std::make_shared<NewCareerScreen>());
            } else if (action == "Settings") {
                m_gameManager->changeScreen(std::make_shared<SettingsScreen>());
            }
        }
        m_pressedIdx = -1;
    }
}

void MenuScreen::update(sf::Time deltaTime) {
    (void)deltaTime;
}

void MenuScreen::draw(sf::RenderWindow& window) {
    auto& font = AssetManager::get().getFont("MainFont");

    UIKit::drawBackground(window);
    UIKit::drawTitle(window, font, {140.f, 140.f}, "Football Simulator", 50);

    // Sub-label under the title, broadcast strap-line style.
    UIKit::drawText(window, font, {142.f, 214.f}, "PLAYER CAREER", 16, UITheme::Accent, 3.0f);

    for (size_t i = 0; i < m_buttons.size(); ++i) {
        UIKit::BtnState st = UIKit::BtnState::Normal;
        if ((int)i == m_pressedIdx)     st = UIKit::BtnState::Pressed;
        else if ((int)i == m_hoverIdx)  st = UIKit::BtnState::Hover;
        UIKit::drawButton(window, font, m_buttons[i].bounds, m_buttons[i].label, st);
    }
}
