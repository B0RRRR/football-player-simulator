#include "SettingsScreen.h"
#include "MenuScreen.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "Settings.h"
#include "UITheme.h"
#include "CareerHubScreen.h"
#include "SaveManager.h"
#include <memory>
#include <iostream>

SettingsScreen::SettingsScreen() {
}

void SettingsScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    m_titleText.setFont(font);
    m_titleText.setString("Settings");
    m_titleText.setCharacterSize(40);
    m_titleText.setFillColor(UITheme::TextWhite);
    m_titleText.setPosition(400.f, 50.f);
    
    m_diffText.setFont(font);
    m_diffText.setCharacterSize(24);
    m_diffText.setFillColor(UITheme::Highlight);
    m_diffText.setPosition(250.f, 150.f);
    updateDifficultyText();
    
    m_speedText.setFont(font);
    m_speedText.setCharacterSize(24);
    m_speedText.setFillColor(UITheme::Highlight);
    m_speedText.setPosition(250.f, 250.f);
    updateSpeedText();
    
    // Setup buttons
    std::vector<std::string> buttonLabels = {"Change Difficulty", "Change Match Speed", "Save Game", "Back to Menu"};
    float startY = 320.f;
    
    for (size_t i = 0; i < buttonLabels.size(); ++i) {
        Button btn;
        btn.rect.setSize(sf::Vector2f(300.f, 50.f));
        btn.rect.setPosition(250.f, startY + i * 80.f);
        btn.rect.setFillColor(UITheme::ButtonNormal);
        
        btn.text.setFont(font);
        btn.text.setString(buttonLabels[i]);
        btn.text.setCharacterSize(24);
        btn.text.setFillColor(UITheme::TextWhite);
        
        sf::FloatRect textRect = btn.text.getLocalBounds();
        btn.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top  + textRect.height/2.0f);
        btn.text.setPosition(
            btn.rect.getPosition().x + btn.rect.getSize().x/2.0f,
            btn.rect.getPosition().y + btn.rect.getSize().y/2.0f
        );
        
        btn.action = buttonLabels[i];
        m_buttons.push_back(btn);
    }
}

void SettingsScreen::updateDifficultyText() {
    std::string diffStr = "Normal";
    if (g_settings.difficulty == 0) diffStr = "Easy";
    if (g_settings.difficulty == 2) diffStr = "Hard";
    m_diffText.setString("Current Difficulty: " + diffStr);
}

void SettingsScreen::updateSpeedText() {
    m_speedText.setString(std::string("Current Match Speed: ") + matchSpeedLabel(g_settings.matchSpeed));
}

void SettingsScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        // Adjust for view scaling
        sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                if (btn.action == "Back to Menu") {
                    if (m_gameManager->getPlayer()->currentClub != nullptr) {
                        m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
                    } else {
                        m_gameManager->changeScreen(std::make_shared<MenuScreen>());
                    }
                } else if (btn.action == "Change Difficulty") {
                    g_settings.difficulty = (g_settings.difficulty + 1) % 3;
                    updateDifficultyText();
                } else if (btn.action == "Change Match Speed") {
                    g_settings.matchSpeed = (g_settings.matchSpeed + 1) % matchSpeedCount();
                    updateSpeedText();
                } else if (btn.action == "Save Game") {
                    if (SaveManager::saveGame("savegame.json", m_gameManager->getPlayer(), m_gameManager->getCareerManager(), &m_gameManager->getDatabase())) {
                        std::cout << "Game saved successfully!\n";
                    }
                }
            }
        }
    }
    
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                btn.rect.setFillColor(UITheme::ButtonHover);
            } else {
                btn.rect.setFillColor(UITheme::ButtonNormal);
            }
        }
    }
}

void SettingsScreen::update(sf::Time deltaTime) {}

void SettingsScreen::draw(sf::RenderWindow& window) {
    UITheme::drawGradientBackground(window);
    window.draw(m_titleText);
    window.draw(m_diffText);
    window.draw(m_speedText);
    for (const auto& btn : m_buttons) {
        window.draw(btn.rect);
        window.draw(btn.text);
    }
}
