#include "UITheme.h"
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
    std::vector<std::string> actions = {"Shooting", "Passing", "Tackling", "Goalkeeping", "Coach", "Car", "Back"};
    float startY = 280.f;
    
    for (size_t i = 0; i < actions.size(); ++i) {
        Button btn;
        
        btn.rect.setSize(sf::Vector2f(400.f, 40.f));
        btn.rect.setPosition(200.f, startY + i * 50.f);
        btn.rect.setFillColor(UITheme::ButtonNormal);
        
        btn.text.setFont(font);
        // Text is set dynamically in update()
        btn.text.setString("");
        btn.text.setCharacterSize(24);
        btn.text.setFillColor(sf::Color::White);
        
        btn.action = actions[i];
        m_buttons.push_back(btn);
    }
}

void UpgradeScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        Player* player = m_gameManager->getPlayer();
        
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                if (btn.action == "Back") {
                    m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
                } else if (btn.action == "Shooting") {
                    int cost = player->shooting * 5;
                    if (player->experience >= cost && player->shooting < 99) {
                        player->experience -= cost;
                        player->shooting++;
                    }
                } else if (btn.action == "Passing") {
                    int cost = player->passing * 5;
                    if (player->experience >= cost && player->passing < 99) {
                        player->experience -= cost;
                        player->passing++;
                    }
                } else if (btn.action == "Tackling") {
                    int cost = player->tackling * 5;
                    if (player->experience >= cost && player->tackling < 99) {
                        player->experience -= cost;
                        player->tackling++;
                    }
                } else if (btn.action == "Goalkeeping") {
                    int cost = player->goalkeeping * 5;
                    if (player->experience >= cost && player->goalkeeping < 99) {
                        player->experience -= cost;
                        player->goalkeeping++;
                    }
                } else if (btn.action == "Coach") {
                    if (player->money >= 25000) {
                        player->money -= 25000;
                        if (player->shooting < 99) player->shooting += 1;
                        if (player->passing < 99) player->passing += 1;
                        if (player->tackling < 99) player->tackling += 1;
                        if (player->goalkeeping < 99) player->goalkeeping += 1;
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
    
    // Hover effects
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

void UpgradeScreen::update(sf::Time deltaTime) {
    Player* p = m_gameManager->getPlayer();
    if (!p) return;
    
    m_xpText.setString("Experience (XP): " + std::to_string(p->experience) + "   Money: $" + std::to_string(p->money));
    
    std::string stats = "Shooting: " + std::to_string(p->shooting) + " | " +
                        "Passing: " + std::to_string(p->passing) + " | " +
                        "Tackling: " + std::to_string(p->tackling) + " | " +
                        "Goalkeeping: " + std::to_string(p->goalkeeping) + "\n" +
                        "Morale: " + std::to_string(p->morale) + "%";
    m_statsText.setString(stats);
    
    // Update button text with dynamic costs
    for (auto& btn : m_buttons) {
        if (btn.action == "Shooting") {
            btn.text.setString("Upgrade Shooting (" + std::to_string(p->shooting * 5) + " XP)");
        } else if (btn.action == "Passing") {
            btn.text.setString("Upgrade Passing (" + std::to_string(p->passing * 5) + " XP)");
        } else if (btn.action == "Tackling") {
            btn.text.setString("Upgrade Tackling (" + std::to_string(p->tackling * 5) + " XP)");
        } else if (btn.action == "Goalkeeping") {
            btn.text.setString("Upgrade Goalkeeping (" + std::to_string(p->goalkeeping * 5) + " XP)");
        } else if (btn.action == "Coach") {
            btn.text.setString("Personal Coach ($25000) [+1 All Stats]");
        } else if (btn.action == "Car") {
            btn.text.setString("Sports Car ($20000) [+50 Morale]");
        } else if (btn.action == "Back") {
            btn.text.setString("Back to Hub");
        }
        
        // Recenter text
        sf::FloatRect textRect = btn.text.getLocalBounds();
        btn.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
        btn.text.setPosition(
            btn.rect.getPosition().x + btn.rect.getSize().x/2.0f,
            btn.rect.getPosition().y + btn.rect.getSize().y/2.0f
        );
    }
}

void UpgradeScreen::draw(sf::RenderWindow& window) {
    UITheme::drawGradientBackground(window);
    
    window.draw(m_titleText);
    window.draw(m_xpText);
    window.draw(m_statsText);
    
    for (const auto& btn : m_buttons) {
        window.draw(btn.rect);
        window.draw(btn.text);
    }
}
