#include "SettingsScreen.h"
#include "MenuScreen.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "Settings.h"
#include "UITheme.h"
#include "UIKit.h"
#include "CareerHubScreen.h"
#include "SaveManager.h"
#include <memory>
#include <iostream>

SettingsScreen::SettingsScreen() {
}

void SettingsScreen::init() {
    m_rows.clear();
    m_hoverIdx = m_pressedIdx = -1;
    m_saveFlash = 0.f;

    struct Def { std::string action, label; RowKind kind; };
    std::vector<Def> defs = {
        {"Difficulty",  "Difficulty",   RowKind::Option},
        {"MatchSpeed",  "Match Speed",  RowKind::Option},
        {"Fullscreen",  "Fullscreen",   RowKind::Option},
        {"Resolution",  "Resolution",   RowKind::Option},
        {"Save",        "Save Game",    RowKind::Action},
        {"Back",        "Back",         RowKind::Action},
    };

    const float x = 140.f, w = 520.f, h = 58.f, gap = 66.f, startY = 280.f;
    for (size_t i = 0; i < defs.size(); ++i) {
        Row r;
        r.bounds = sf::FloatRect(x, startY + i * gap, w, h);
        r.action = defs[i].action;
        r.label = defs[i].label;
        r.kind = defs[i].kind;
        m_rows.push_back(r);
    }
}

std::string SettingsScreen::currentValue(const std::string& action) const {
    if (action == "Difficulty") {
        if (g_settings.difficulty == 0) return "Easy";
        if (g_settings.difficulty == 2) return "Hard";
        return "Normal";
    }
    if (action == "MatchSpeed") return matchSpeedLabel(g_settings.matchSpeed);
    if (action == "Fullscreen") return g_settings.isFullscreen ? "On" : "Off";
    if (action == "Resolution")
        return std::to_string(g_settings.resWidth) + " x " + std::to_string(g_settings.resHeight);
    return "";
}

void SettingsScreen::cycleResolution() {
    static const unsigned presets[][2] = {{1280, 720}, {1600, 900}, {1920, 1080}};
    const int n = 3;
    int cur = 0;
    for (int i = 0; i < n; ++i)
        if (presets[i][0] == g_settings.resWidth && presets[i][1] == g_settings.resHeight) cur = i;
    int next = (cur + 1) % n;
    g_settings.resWidth = presets[next][0];
    g_settings.resHeight = presets[next][1];
    g_settings.isFullscreen = false;  // choosing a resolution implies windowed
}

void SettingsScreen::activate(const std::string& action) {
    if (action == "Difficulty") {
        g_settings.difficulty = (g_settings.difficulty + 1) % 3;
    } else if (action == "MatchSpeed") {
        g_settings.matchSpeed = (g_settings.matchSpeed + 1) % matchSpeedCount();
    } else if (action == "Fullscreen") {
        g_settings.isFullscreen = !g_settings.isFullscreen;
        m_gameManager->requestVideoApply();
    } else if (action == "Resolution") {
        cycleResolution();
        m_gameManager->requestVideoApply();
    } else if (action == "Save") {
        if (SaveManager::saveGame("savegame.json", m_gameManager->getPlayer(),
                                  m_gameManager->getCareerManager(), &m_gameManager->getDatabase())) {
            m_saveFlash = 2.0f;
            std::cout << "Game saved successfully!\n";
        }
    } else if (action == "Back") {
        if (m_gameManager->getPlayer()->currentClub != nullptr)
            m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
        else
            m_gameManager->changeScreen(std::make_shared<MenuScreen>());
    }
}

void SettingsScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        m_hoverIdx = -1;
        for (size_t i = 0; i < m_rows.size(); ++i)
            if (m_rows[i].bounds.contains(m)) m_hoverIdx = (int)i;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        m_pressedIdx = -1;
        for (size_t i = 0; i < m_rows.size(); ++i)
            if (m_rows[i].bounds.contains(m)) m_pressedIdx = (int)i;
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        int released = -1;
        for (size_t i = 0; i < m_rows.size(); ++i)
            if (m_rows[i].bounds.contains(m)) released = (int)i;
        if (released >= 0 && released == m_pressedIdx) activate(m_rows[released].action);
        m_pressedIdx = -1;
    }
}

void SettingsScreen::update(sf::Time deltaTime) {
    if (m_saveFlash > 0.f) m_saveFlash -= deltaTime.asSeconds();
}

void SettingsScreen::draw(sf::RenderWindow& window) {
    auto& font = AssetManager::get().getFont("MainFont");

    UIKit::drawBackground(window);
    UIKit::drawTitle(window, font, {140.f, 150.f}, "Settings", 46);

    for (size_t i = 0; i < m_rows.size(); ++i) {
        UIKit::BtnState st = UIKit::BtnState::Normal;
        if ((int)i == m_pressedIdx)     st = UIKit::BtnState::Pressed;
        else if ((int)i == m_hoverIdx)  st = UIKit::BtnState::Hover;

        if (m_rows[i].kind == SettingsScreen::RowKind::Option)
            UIKit::drawOptionRow(window, font, m_rows[i].bounds, m_rows[i].label,
                                 currentValue(m_rows[i].action), st);
        else
            UIKit::drawButton(window, font, m_rows[i].bounds, m_rows[i].label, st);
    }

    if (m_saveFlash > 0.f)
        UIKit::drawText(window, font, {140.f, 674.f}, "Game saved", 18, UITheme::Accent, 2.0f);
}
