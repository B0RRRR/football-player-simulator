#include "UpgradeScreen.h"
#include "CareerHubScreen.h"
#include "AssetManager.h"
#include "GameManager.h"
#include <iostream>

UpgradeScreen::UpgradeScreen() {
}

void UpgradeScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    m_titleText.setFont(font);
    m_titleText.setString("Training & Upgrades");
    m_titleText.setCharacterSize(40);
    m_titleText.setFillColor(sf::Color::White);
    m_titleText.setPosition(200.f, 50.f);
    
    m_xpText.setFont(font);
    m_xpText.setCharacterSize(26);
    m_xpText.setFillColor(sf::Color::Yellow);
    m_xpText.setPosition(200.f, 100.f);
    
    m_statsText.setFont(font);
    m_statsText.setCharacterSize(20);
    m_statsText.setFillColor(sf::Color::White);
    m_statsText.setPosition(200.f, 140.f);
    
    // Setup buttons
    std::vector<std::string> buttonLabels = {
        "Upgrade Shooting (100 XP)", 
        "Upgrade Passing (100 XP)", 
        "Upgrade Tackling (100 XP)", 
        "Upgrade Goalkeeping (100 XP)", 
        "Personal Coach ($5000) [+5 All Stats]",
        "Sports Car ($20000) [+50 Morale]",
        "Back to Menu"
    };
    std::vector<std::string> actions = {"Shooting", "Passing", "Tackling", "Goalkeeping", "Coach", "Car", "Back"};
    float startY = 280.f;
    
    for (size_t i = 0; i < buttonLabels.size(); ++i) {
        Button btn;
        
        btn.rect.setSize(sf::Vector2f(400.f, 40.f));
        btn.rect.setPosition(200.f, startY + i * 50.f);
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
        
        btn.action = actions[i];
        m_buttons.push_back(btn);
    }
}

void UpgradeScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
        Player* player = m_gameManager->getPlayer();
        
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                if (btn.action == "Back") {
                    m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
                } else if (btn.action == "Shooting") {
                    if (player->experience >= UPGRADE_COST && player->shooting < 100) {
                        player->experience -= UPGRADE_COST;
                        player->shooting++;
                    }
                } else if (btn.action == "Passing") {
                    if (player->experience >= UPGRADE_COST && player->passing < 100) {
                        player->experience -= UPGRADE_COST;
                        player->passing++;
                    }
                } else if (btn.action == "Tackling") {
                    if (player->experience >= UPGRADE_COST && player->tackling < 100) {
                        player->experience -= UPGRADE_COST;
                        player->tackling++;
                    }
                } else if (btn.action == "Goalkeeping") {
                    if (player->experience >= UPGRADE_COST && player->goalkeeping < 100) {
                        player->experience -= UPGRADE_COST;
                        player->goalkeeping++;
                    }
                } else if (btn.action == "Coach") {
                    if (player->money >= 5000) {
                        player->money -= 5000;
                        player->shooting += 5;
                        player->passing += 5;
                        player->tackling += 5;
                        player->goalkeeping += 5;
                    }
                } else if (btn.action == "Car") {
                    if (player->money >= 20000) {
                        player->money -= 20000;
                        player->morale += 50;
                        if (player->morale > 100) player->morale = 100;
                    }
                }
            }
        }
    }
    
    // Hover effect
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

void UpgradeScreen::update(sf::Time deltaTime) {
    Player* p = m_gameManager->getPlayer();
    m_xpText.setString("XP: " + std::to_string(p->experience) + " | Money: $" + std::to_string(p->money));
    std::string s = "Shooting: " + std::to_string(p->shooting) + " | Passing: " + std::to_string(p->passing) + "\n";
    s += "Tackling: " + std::to_string(p->tackling) + " | Goalkeeping: " + std::to_string(p->goalkeeping) + "\n";
    s += "Morale: " + std::to_string(p->morale);
    m_statsText.setString(s);
}

void UpgradeScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(30, 30, 30));
    
    window.draw(m_titleText);
    window.draw(m_xpText);
    window.draw(m_statsText);
    
    for (const auto& btn : m_buttons) {
        window.draw(btn.rect);
        window.draw(btn.text);
    }
}
