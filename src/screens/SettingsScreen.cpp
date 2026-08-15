#include "SettingsScreen.h"
#include "MenuScreen.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "Settings.h"
#include "UITheme.h"
#include "UIKit.h"
#include "AudioManager.h"
#include "CareerHubScreen.h"
#include "SaveManager.h"
#include <memory>
#include <iostream>
#include <cmath>
#include <algorithm>

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
        {"Music",       "Music Volume", RowKind::Slider},
        {"Sound",       "Sound Volume", RowKind::Slider},
        {"Fullscreen",  "Fullscreen",   RowKind::Option},
        {"Resolution",  "Resolution",   RowKind::Option},
        {"Save",        "Save Game",    RowKind::Action},
        {"Back",        "Back",         RowKind::Action},
    };

    const float w = 520.f, x = (1280.f - w) / 2.f, h = 52.f, gap = 60.f, startY = 176.f;
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
    if (action == "Music") return std::to_string(g_settings.musicVolume) + "%";
    if (action == "Sound") return std::to_string(g_settings.soundVolume) + "%";
    if (action == "Fullscreen") return g_settings.isFullscreen ? "On" : "Off";
    if (action == "Resolution")
        return std::to_string(g_settings.resWidth) + " x " + std::to_string(g_settings.resHeight);
    return "";
}

void SettingsScreen::cycleResolution() {
    static const unsigned presets[][2] = {{1280, 720}, {1600, 900}, {1920, 1080}, {2560, 1440}};
    const int n = 4;
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
            AudioManager::get().sfx("confirm");
        }
    } else if (action == "Back") {
        g_settings.save();
        if (m_gameManager->getPlayer()->currentClub != nullptr)
            m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
        else
            m_gameManager->changeScreen(std::make_shared<MenuScreen>());
        return;
    }
    g_settings.save(); // persist any option change (difficulty, speed, fullscreen, resolution)
}

void SettingsScreen::setVolumeFromMouse(const std::string& action, float mouseX) {
    for (auto& r : m_rows) {
        if (r.action != action) continue;
        sf::FloatRect tr = UIKit::sliderTrack(r.bounds);
        float v = std::clamp((mouseX - tr.left) / tr.width, 0.f, 1.f);
        int vol = (int)std::lround(v * 100.f);
        if (action == "Music") g_settings.musicVolume = vol;
        else                   g_settings.soundVolume = vol;
        AudioManager::get().applyVolumes();
        return;
    }
}

void SettingsScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    // A generous grab band around a slider's track (so the knob is easy to catch).
    auto sliderGrab = [&](const Row& r) {
        sf::FloatRect tr = UIKit::sliderTrack(r.bounds);
        return sf::FloatRect(tr.left - 12.f, r.bounds.top, tr.width + 24.f, r.bounds.height);
    };

    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        if (!m_dragAction.empty()) { setVolumeFromMouse(m_dragAction, m.x); return; }
        m_hoverIdx = -1;
        for (size_t i = 0; i < m_rows.size(); ++i)
            if (m_rows[i].bounds.contains(m)) m_hoverIdx = (int)i;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f m = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        // Start dragging if the press landed on a slider.
        for (auto& r : m_rows)
            if (r.kind == RowKind::Slider && sliderGrab(r).contains(m)) {
                m_dragAction = r.action;
                setVolumeFromMouse(m_dragAction, m.x);
                m_pressedIdx = -1;
                return;
            }
        m_pressedIdx = -1;
        for (size_t i = 0; i < m_rows.size(); ++i)
            if (m_rows[i].bounds.contains(m)) m_pressedIdx = (int)i;
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        if (!m_dragAction.empty()) {
            if (m_dragAction == "Sound") AudioManager::get().sfx("confirm"); // preview level
            m_dragAction.clear();
            g_settings.save(); // persist the new volume
            return;
        }
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
    // Centre the title block (accent bar + text) on the screen's mid-line.
    float tw = UIKit::crispText(font, UIKit::upper("Settings"), 44).getGlobalBounds().width;
    UIKit::drawTitle(window, font, {652.f - tw / 2.f, 74.f}, "Settings", 44);

    for (size_t i = 0; i < m_rows.size(); ++i) {
        const Row& r = m_rows[i];
        UIKit::BtnState st = UIKit::BtnState::Normal;
        if ((int)i == m_pressedIdx)     st = UIKit::BtnState::Pressed;
        else if ((int)i == m_hoverIdx)  st = UIKit::BtnState::Hover;
        if (r.action == m_dragAction)   st = UIKit::BtnState::Pressed; // active drag

        if (r.kind == RowKind::Slider) {
            float v = (r.action == "Music" ? g_settings.musicVolume : g_settings.soundVolume) / 100.f;
            UIKit::drawSlider(window, font, r.bounds, r.label, v, st);
        } else if (r.kind == RowKind::Option) {
            UIKit::drawOptionRow(window, font, r.bounds, r.label, currentValue(r.action), st);
        } else {
            UIKit::drawButton(window, font, r.bounds, r.label, st);
        }
    }

    if (m_saveFlash > 0.f) {
        float sw = UIKit::crispText(font, "Game saved", 18).getGlobalBounds().width;
        UIKit::drawText(window, font, {640.f - sw / 2.f, 678.f}, "Game saved", 18, UITheme::Accent, 2.0f);
    }
}
