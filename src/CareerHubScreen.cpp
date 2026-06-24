#include "CareerHubScreen.h"
#include "MatchScreen.h"
#include "MenuScreen.h"
#include "UpgradeScreen.h"
#include "LeagueTableScreen.h"
#include "TrainingScreen.h"
#include "EventScreen.h"
#include "TransferScreen.h"
#include "SeasonEndScreen.h"
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
    float startY = 350.f;
    
    for (size_t i = 0; i < buttonLabels.size(); ++i) {
        Button btn;
        btn.rect.setSize(sf::Vector2f(300.f, 50.f));
        btn.rect.setPosition(50.f, startY + i * 60.f);
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
                } else if (btn.action == "View Offers") {
                    m_gameManager->changeScreen(std::make_shared<TransferScreen>());
                } else if (btn.action == "Advance Day" || btn.action == "Recovery") {
                    Player* p = m_gameManager->getPlayer();
                    CareerManager* cm = m_gameManager->getCareerManager();
                    
                    if (p->injuredDays > 0) {
                        // Injured, just rest and advance day
                        p->energy += 20; // extra rest
                        if (p->energy > 100) p->energy = 100;
                        cm->advanceDay();
                        return; // exit loop
                    }

                    // 15% chance of random event on any day
                    if (rand() % 100 < 15) {
                        m_gameManager->changeScreen(std::make_shared<EventScreen>());
                        return;
                    }

                    if (cm->getDayType() == CalendarDayType::Match) {
                        m_gameManager->changeScreen(std::make_shared<MatchScreen>());
                    } else if (cm->getDayType() == CalendarDayType::Training) {
                        m_gameManager->changeScreen(std::make_shared<TrainingScreen>());
                    } else {
                        // Rest
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
    
    if (p && cm && p->weeksPlayed >= cm->getSeasonLength()) {
        m_gameManager->changeScreen(std::make_shared<SeasonEndScreen>());
        return;
    }
    
    if (p && p->currentClub) {
        std::string titleStr = p->currentClub->name + " - Hub";
        m_titleText.setString(sf::String::fromUtf8(titleStr.begin(), titleStr.end()));
        
        std::string stats = "Name: " + p->name + "\n";
        stats += "Shooting: " + std::to_string(p->shooting) + "\n";
        stats += "Passing: " + std::to_string(p->passing) + "\n";
        stats += "Morale: " + std::to_string(p->morale) + "\n";
        stats += "Energy: " + std::to_string(p->energy) + "%\n";
        stats += "XP: " + std::to_string(p->experience) + "\n";
        stats += "Money: $" + std::to_string(p->money) + "\n";
        stats += "Salary: $" + std::to_string(p->salary) + "/w\n";
        m_playerStatsText.setString(stats);
        
        if (p->injuredDays > 0) {
            m_buttons[0].text.setString("Recovery (Rest)");
            m_buttons[0].action = "Recovery";
            m_buttons[0].rect.setFillColor(sf::Color(150, 50, 50));
        } else {
            m_buttons[0].text.setString("Advance Day");
            m_buttons[0].action = "Advance Day";
        }

        // Handle Transfer Window
        if (p->weeksPlayed > 0 && p->weeksPlayed % 15 == 0) {
            bool hasTransferBtn = false;
            for (auto& b : m_buttons) if (b.action == "View Offers") hasTransferBtn = true;
            
            if (!hasTransferBtn) {
                Button btnOff;
                btnOff.rect.setSize(sf::Vector2f(200.f, 40.f));
                btnOff.rect.setPosition(450.f, 300.f);
                btnOff.rect.setFillColor(sf::Color(200, 150, 50));
                
                auto& font = AssetManager::get().getFont("MainFont");
                btnOff.text.setFont(font);
                btnOff.text.setString("View Offers");
                btnOff.text.setCharacterSize(18);
                btnOff.text.setFillColor(sf::Color::Black);
                sf::FloatRect tr = btnOff.text.getLocalBounds();
                btnOff.text.setOrigin(tr.left + tr.width/2.0f, tr.top + tr.height/2.0f);
                btnOff.text.setPosition(btnOff.rect.getPosition().x + btnOff.rect.getSize().x/2.0f,
                                        btnOff.rect.getPosition().y + btnOff.rect.getSize().y/2.0f);
                btnOff.action = "View Offers";
                m_buttons.push_back(btnOff);
            }
        }
    }
    
    if (cm) {
        std::string cal = "Day: " + std::to_string(cm->getCurrentDay()) + " (Week " + std::to_string(p ? p->weeksPlayed : 0) + ")\n";
        cal += "Schedule: " + cm->getDayTypeString() + "\n";
        if (p && p->weeksPlayed > 0 && p->weeksPlayed % 15 == 0) {
            cal += "\nTRANSFER WINDOW OPEN!";
        }
        if (p && p->injuredDays > 0) {
            cal += "\nINJURED: " + std::to_string(p->injuredDays) + " days left";
            m_calendarText.setFillColor(sf::Color::Red);
        } else if (p && p->weeksPlayed > 0 && p->weeksPlayed % 15 == 0) {
            m_calendarText.setFillColor(sf::Color::Yellow);
        } else {
            m_calendarText.setFillColor(sf::Color::Yellow);
        }
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
