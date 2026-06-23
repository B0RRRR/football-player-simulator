#include "CareerHubScreen.h"
#include "MatchScreen.h"
#include "MenuScreen.h"
#include "UpgradeScreen.h"
#include "LeagueTableScreen.h"
#include "TrainingScreen.h"
#include "GameManager.h"
#include "AssetManager.h"

CareerHubScreen::CareerHubScreen() {
}

void CareerHubScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    m_titleText.setFont(font);
    m_titleText.setCharacterSize(40);
    m_titleText.setFillColor(sf::Color::White);
    m_titleText.setPosition(50.f, 30.f);
    
    m_playerStatsText.setFont(font);
    m_playerStatsText.setCharacterSize(24);
    m_playerStatsText.setFillColor(sf::Color(200, 200, 200));
    m_playerStatsText.setPosition(50.f, 100.f);

    m_calendarText.setFont(font);
    m_calendarText.setCharacterSize(30);
    m_calendarText.setFillColor(sf::Color::Yellow);
    m_calendarText.setPosition(450.f, 100.f);
    
    std::vector<std::string> buttonLabels = {"Advance Day", "Upgrades", "League Table", "Quit to Menu"};
    float startY = 300.f;
    
    for (size_t i = 0; i < buttonLabels.size(); ++i) {
        Button btn;
        btn.rect.setSize(sf::Vector2f(300.f, 50.f));
        btn.rect.setPosition(50.f, startY + i * 70.f);
        btn.rect.setFillColor(sf::Color(100, 100, 100));
        
        btn.text.setFont(font);
        btn.text.setString(buttonLabels[i]);
        btn.text.setCharacterSize(20);
        btn.text.setFillColor(sf::Color::White);
        
        sf::FloatRect textRect = btn.text.getLocalBounds();
        btn.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
        btn.text.setPosition(btn.rect.getPosition().x + btn.rect.getSize().x/2.0f,
                             btn.rect.getPosition().y + btn.rect.getSize().y/2.0f);
        
        btn.action = buttonLabels[i];
        m_buttons.push_back(btn);
    }
}

void CareerHubScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
        
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                if (btn.action == "Quit to Menu") {
                    m_gameManager->changeScreen(std::make_shared<MenuScreen>());
                } else if (btn.action == "Advance Day") {
                    CareerManager* cm = m_gameManager->getCareerManager();
                    if (cm->getDayType() == CalendarDayType::Match) {
                        m_gameManager->changeScreen(std::make_shared<MatchScreen>());
                    } else if (cm->getDayType() == CalendarDayType::Training) {
                        m_gameManager->changeScreen(std::make_shared<TrainingScreen>());
                    } else {
                        // Training or Rest
                        cm->advanceDay();
                    }
                } else if (btn.action == "Upgrades") {
                    m_gameManager->changeScreen(std::make_shared<UpgradeScreen>());
                } else if (btn.action == "League Table") {
                    m_gameManager->changeScreen(std::make_shared<LeagueTableScreen>());
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

void CareerHubScreen::update(sf::Time deltaTime) {
    Player* p = m_gameManager->getPlayer();
    CareerManager* cm = m_gameManager->getCareerManager();
    
    if (p && p->currentClub) {
        m_titleText.setString(p->currentClub->name + " - Hub");
        
        std::string stats = "Name: " + p->name + "\n";
        stats += "Shooting: " + std::to_string(p->shooting) + "\n";
        stats += "Passing: " + std::to_string(p->passing) + "\n";
        stats += "Energy: " + std::to_string(p->energy) + "%\n";
        stats += "XP: " + std::to_string(p->experience) + "\n";
        m_playerStatsText.setString(stats);
    }
    
    if (cm) {
        std::string cal = "Day: " + std::to_string(cm->getCurrentDay()) + "\n";
        cal += "Schedule: " + cm->getDayTypeString();
        m_calendarText.setString(cal);
    }
}

void CareerHubScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(20, 30, 20));
    window.draw(m_titleText);
    window.draw(m_playerStatsText);
    window.draw(m_calendarText);
    
    for (const auto& btn : m_buttons) {
        window.draw(btn.rect);
        window.draw(btn.text);
    }
}
