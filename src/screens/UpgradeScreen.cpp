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
    
    // Only offer the attributes this position actually uses - an outfielder has no reason
    // to train goalkeeping, and a keeper none to train shooting.
    Player* pl = m_gameManager->getPlayer();
    std::vector<std::string> actions;
    if (!pl || pl->usesShooting())    actions.push_back("Shooting");
    if (!pl || pl->usesPassing())     actions.push_back("Passing");
    if (!pl || pl->usesTackling())    actions.push_back("Tackling");
    if (!pl || pl->usesGoalkeeping()) actions.push_back("Goalkeeping");
    actions.push_back("Coach");
    actions.push_back("Car");
    actions.push_back("Back");
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
                    if (player->experience >= cost && player->shooting < player->potential) {
                        player->experience -= cost;
                        player->shooting++;
                    }
                } else if (btn.action == "Passing") {
                    int cost = player->passing * 5;
                    if (player->experience >= cost && player->passing < player->potential) {
                        player->experience -= cost;
                        player->passing++;
                    }
                } else if (btn.action == "Tackling") {
                    int cost = player->tackling * 5;
                    if (player->experience >= cost && player->tackling < player->potential) {
                        player->experience -= cost;
                        player->tackling++;
                    }
                } else if (btn.action == "Goalkeeping") {
                    int cost = player->goalkeeping * 5;
                    if (player->experience >= cost && player->goalkeeping < player->potential) {
                        player->experience -= cost;
                        player->goalkeeping++;
                    }
                } else if (btn.action == "Coach") {
                    if (player->money >= 25000) {
                        player->money -= 25000;
                        // Only the attributes he actually trains.
                        if (player->usesShooting()    && player->shooting    < player->potential) player->shooting    += 1;
                        if (player->usesPassing()     && player->passing     < player->potential) player->passing     += 1;
                        if (player->usesTackling()    && player->tackling    < player->potential) player->tackling    += 1;
                        if (player->usesGoalkeeping() && player->goalkeeping < player->potential) player->goalkeeping += 1;
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
    
    std::string stats;
    if (p->usesShooting())    stats += "Shooting: " + std::to_string(p->shooting) + " | ";
    if (p->usesPassing())     stats += "Passing: " + std::to_string(p->passing) + " | ";
    if (p->usesTackling())    stats += "Tackling: " + std::to_string(p->tackling) + " | ";
    if (p->usesGoalkeeping()) stats += "Goalkeeping: " + std::to_string(p->goalkeeping) + " | ";
    if (stats.size() >= 3) stats.erase(stats.size() - 3);
    stats += "\nOverall: " + std::to_string(p->overall()) + "   Potential (cap): " + std::to_string(p->potential) +
             "   Morale: " + std::to_string(p->morale) + "%";
    m_statsText.setString(stats);

    // Each upgrade button shows its cost, or MAX once the stat has hit the potential cap.
    auto label = [&](const char* n, int stat) {
        return (stat >= p->potential)
            ? std::string("Upgrade ") + n + " (MAX)"
            : std::string("Upgrade ") + n + " (" + std::to_string(stat * 5) + " XP)";
    };
    for (auto& btn : m_buttons) {
        if (btn.action == "Shooting") {
            btn.text.setString(label("Shooting", p->shooting));
        } else if (btn.action == "Passing") {
            btn.text.setString(label("Passing", p->passing));
        } else if (btn.action == "Tackling") {
            btn.text.setString(label("Tackling", p->tackling));
        } else if (btn.action == "Goalkeeping") {
            btn.text.setString(label("Goalkeeping", p->goalkeeping));
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
