#include "MenuScreen.h"
#include "SettingsScreen.h"
#include "MatchScreen.h"
#include "NewCareerScreen.h"
#include "UpgradeScreen.h"
#include "AssetManager.h"
#include "GameManager.h"
#include "UITheme.h"
#include "SaveManager.h"
#include "CareerHubScreen.h"
#include <iostream>

MenuScreen::MenuScreen() {
}

void MenuScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    // Setup title
    m_titleText.setFont(font);
    m_titleText.setString("Football Simulator");
    m_titleText.setCharacterSize(50);
    m_titleText.setFillColor(sf::Color::White);
    m_titleText.setPosition(200.f, 100.f);
    
    // Setup buttons
    std::vector<std::string> buttonLabels;
    if (SaveManager::hasSaveGame("savegame.json")) {
        buttonLabels.push_back("Continue Career");
    }
    buttonLabels.push_back("New Career");
    buttonLabels.push_back("Settings");
    buttonLabels.push_back("Exit");
    float startY = 250.f;
    
    for (size_t i = 0; i < buttonLabels.size(); ++i) {
        Button btn;
        
        btn.rect.setSize(sf::Vector2f(300.f, 50.f));
        btn.rect.setPosition(250.f, startY + i * 80.f);
        btn.rect.setFillColor(UITheme::ButtonNormal);
        
        btn.text.setFont(font);
        btn.text.setString(buttonLabels[i]);
        btn.text.setCharacterSize(24);
        btn.text.setFillColor(sf::Color::White);
        
        // Center text in button
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

void MenuScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
            
            for (auto& btn : m_buttons) {
                if (btn.rect.getGlobalBounds().contains(mousePos)) {
                    if (btn.action == "Exit") {
                        window.close();
                    } else if (btn.action == "Continue Career") {
                        if (SaveManager::loadGame("savegame.json", m_gameManager->getPlayer(), m_gameManager->getCareerManager(), &m_gameManager->getDatabase())) {
                            m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
                        } else {
                            std::cout << "Failed to load savegame.\n";
                        }
                    } else if (btn.action == "New Career") {
                        m_gameManager->changeScreen(std::make_shared<NewCareerScreen>());
                    } else if (btn.action == "Settings") {
                        m_gameManager->changeScreen(std::make_shared<SettingsScreen>());
                    }
                }
            }
        }
    }
    
    // Hover effect
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2i pixelPos(event.mouseMove.x, event.mouseMove.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                btn.rect.setFillColor(UITheme::ButtonNormal);
            } else {
                btn.rect.setFillColor(UITheme::ButtonNormal);
            }
        }
    }
}

void MenuScreen::update(sf::Time deltaTime) {
    // Menu logic
}

void MenuScreen::draw(sf::RenderWindow& window) {
    UITheme::drawGradientBackground(window); // Dark background
    
    window.draw(m_titleText);
    
    for (const auto& btn : m_buttons) {
        window.draw(btn.rect);
        window.draw(btn.text);
    }
}
