#include "SettingsScreen.h"
#include "MenuScreen.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "Settings.h"
#include <memory>

SettingsScreen::SettingsScreen() {
}

void SettingsScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    m_titleText.setFont(font);
    m_titleText.setString("Settings");
    m_titleText.setCharacterSize(40);
    m_titleText.setFillColor(sf::Color::White);
    m_titleText.setPosition(300.f, 50.f);
    
    m_diffText.setFont(font);
    m_diffText.setCharacterSize(24);
    m_diffText.setFillColor(sf::Color::Yellow);
    m_diffText.setPosition(250.f, 200.f);
    updateDifficultyText();
    
    // Setup buttons
    std::vector<std::string> buttonLabels = {"Change Difficulty", "Back to Menu"};
    float startY = 300.f;
    
    for (size_t i = 0; i < buttonLabels.size(); ++i) {
        Button btn;
        btn.rect.setSize(sf::Vector2f(300.f, 50.f));
        btn.rect.setPosition(250.f, startY + i * 80.f);
        btn.rect.setFillColor(sf::Color(100, 100, 100));
        
        btn.text.setFont(font);
        btn.text.setString(buttonLabels[i]);
        btn.text.setCharacterSize(24);
        btn.text.setFillColor(sf::Color::White);
        
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

void SettingsScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                if (btn.action == "Back to Menu") {
                    m_gameManager->changeScreen(std::make_shared<MenuScreen>());
                } else if (btn.action == "Change Difficulty") {
                    g_settings.difficulty = (g_settings.difficulty + 1) % 3;
                    updateDifficultyText();
                }
            }
        }
    }
    
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos(event.mouseMove.x, event.mouseMove.y);
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                btn.rect.setFillColor(sf::Color(150, 150, 150));
            } else {
                btn.rect.setFillColor(sf::Color(100, 100, 100));
            }
        }
    }
}

void SettingsScreen::update(sf::Time deltaTime) {}

void SettingsScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(40, 40, 60)); // Slightly blueish dark background
    window.draw(m_titleText);
    window.draw(m_diffText);
    for (const auto& btn : m_buttons) {
        window.draw(btn.rect);
        window.draw(btn.text);
    }
}
