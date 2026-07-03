#include "UITheme.h"
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
#include "MatchEngine.h"
#include "MatchStatsScreen.h"
#include "MatchEngine.h"
#include "MatchStatsScreen.h"
#include "EuropeanCupScreen.h"
#include "SettingsScreen.h"

CareerHubScreen::CareerHubScreen() {
}

void CareerHubScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    m_titleText.setFont(font);
    m_titleText.setCharacterSize(40);
    m_titleText.setFillColor(sf::Color::White);
    m_titleText.setPosition(50.f, 30.f);
    
    m_btnSettings.rect.setSize(sf::Vector2f(120.f, 40.f));
    m_btnSettings.rect.setPosition(1100.f, 20.f);
    m_btnSettings.baseColor = sf::Color(100, 100, 100);
    m_btnSettings.rect.setFillColor(m_btnSettings.baseColor);
    
    m_btnSettings.text.setFont(font);
    m_btnSettings.text.setString("Settings");
    m_btnSettings.text.setCharacterSize(18);
    m_btnSettings.text.setFillColor(sf::Color::White);
    sf::FloatRect sr = m_btnSettings.text.getLocalBounds();
    m_btnSettings.text.setOrigin(sr.left + sr.width/2.0f, sr.top + sr.height/2.0f);
    m_btnSettings.text.setPosition(m_btnSettings.rect.getPosition().x + m_btnSettings.rect.getSize().x/2.0f,
                                   m_btnSettings.rect.getPosition().y + m_btnSettings.rect.getSize().y/2.0f);
    m_btnSettings.action = "Settings";
    
    m_playerStatsText.setFont(font);
    m_playerStatsText.setCharacterSize(24);
    m_playerStatsText.setFillColor(sf::Color(200, 200, 200));
    m_playerStatsText.setPosition(50.f, 100.f);

    m_calendarText.setFont(font);
    m_calendarText.setCharacterSize(24);
    m_calendarText.setFillColor(sf::Color::Yellow);
    m_calendarText.setPosition(400.f, 100.f);
    
    std::vector<std::string> buttonLabels = {"Advance Day", "Upgrades", "League Table", "Tournaments", "Quit to Menu"};
    float startY = 350.f;
    
    for (size_t i = 0; i < buttonLabels.size(); ++i) {
        Button btn;
        btn.rect.setSize(sf::Vector2f(300.f, 50.f));
        btn.rect.setPosition(50.f, startY + i * 60.f);
        btn.baseColor = UITheme::ButtonNormal;
        
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
    
    // DEBUG BUTTONS
    Button debugMatch;
    debugMatch.rect.setSize(sf::Vector2f(300.f, 50.f));
    debugMatch.rect.setPosition(450.f, 350.f);
    debugMatch.baseColor = sf::Color(150, 50, 50);
    
    debugMatch.text.setFont(font);
    debugMatch.text.setString("Debug: Skip Match");
    debugMatch.text.setCharacterSize(20);
    debugMatch.text.setFillColor(sf::Color::White);
    
    sf::FloatRect tr1 = debugMatch.text.getLocalBounds();
    debugMatch.text.setOrigin(tr1.left + tr1.width/2.0f, tr1.top + tr1.height/2.0f);
    debugMatch.text.setPosition(debugMatch.rect.getPosition().x + debugMatch.rect.getSize().x/2.0f,
                                debugMatch.rect.getPosition().y + debugMatch.rect.getSize().y/2.0f);
    
    debugMatch.action = "Debug: Skip Match";
    m_buttons.push_back(debugMatch);
    
    Button debugTrain;
    debugTrain.rect.setSize(sf::Vector2f(300.f, 50.f));
    debugTrain.rect.setPosition(450.f, 410.f);
    debugTrain.baseColor = sf::Color(150, 50, 50);
    
    debugTrain.text.setFont(font);
    debugTrain.text.setString("Debug: Skip Training");
    debugTrain.text.setCharacterSize(20);
    debugTrain.text.setFillColor(sf::Color::White);
    
    sf::FloatRect tr2 = debugTrain.text.getLocalBounds();
    debugTrain.text.setOrigin(tr2.left + tr2.width/2.0f, tr2.top + tr2.height/2.0f);
    debugTrain.text.setPosition(debugTrain.rect.getPosition().x + debugTrain.rect.getSize().x/2.0f,
                                debugTrain.rect.getPosition().y + debugTrain.rect.getSize().y/2.0f);
    
    debugTrain.action = "Debug: Skip Training";
    m_buttons.push_back(debugTrain);
    
    Button debugSeason;
    debugSeason.rect.setSize(sf::Vector2f(300.f, 50.f));
    debugSeason.rect.setPosition(450.f, 470.f);
    debugSeason.baseColor = sf::Color(150, 50, 50);
    
    debugSeason.text.setFont(font);
    debugSeason.text.setString("Debug: Skip Season");
    debugSeason.text.setCharacterSize(20);
    debugSeason.text.setFillColor(sf::Color::White);
    
    sf::FloatRect tr3 = debugSeason.text.getLocalBounds();
    debugSeason.text.setOrigin(tr3.left + tr3.width/2.0f, tr3.top + tr3.height/2.0f);
    debugSeason.text.setPosition(debugSeason.rect.getPosition().x + debugSeason.rect.getSize().x/2.0f,
                                 debugSeason.rect.getPosition().y + debugSeason.rect.getSize().y/2.0f);
                                 
    debugSeason.action = "Debug: Skip Season";
    m_buttons.push_back(debugSeason);
}

void CareerHubScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        
        if (m_btnSettings.rect.getGlobalBounds().contains(mousePos)) {
            m_gameManager->changeScreen(std::make_shared<SettingsScreen>());
            return;
        }
        
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                if (btn.action == "Tournaments" || btn.action == "European Cups") {
                    m_gameManager->changeScreen(std::make_shared<EuropeanCupScreen>());
                } else if (btn.action == "European Cups") {
                    m_gameManager->changeScreen(std::make_shared<EuropeanCupScreen>());
                } else if (btn.action == "European Cups") {
                    m_gameManager->changeScreen(std::make_shared<EuropeanCupScreen>());
                } else if (btn.action == "Quit to Menu") {
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

                    // 15% chance of random event ONLY on Rest days
                    if (cm->getDayType() == CalendarDayType::Rest && (rand() % 100 < 15)) {
                        m_gameManager->changeScreen(std::make_shared<EventScreen>());
                        return;
                    }

                    if (cm->isSummerBreak()) {
                        if (cm->hasInternationalMatchToday()) {
                            m_gameManager->changeScreen(std::make_shared<MatchScreen>());
                        } else {
                            cm->advanceDay();
                        }
                    } else if (cm->getDayType() == CalendarDayType::Match) {
                        m_gameManager->changeScreen(std::make_shared<MatchScreen>());
                    } else if (cm->getDayType() == CalendarDayType::Training) {
                        m_gameManager->changeScreen(std::make_shared<TrainingScreen>());
                    } else {
                        // Rest
                        cm->advanceDay();
                    }
                } else if (btn.action == "Skip Summer") {
                    CareerManager* cm = m_gameManager->getCareerManager();
                    while (cm->isSummerBreak()) {
                        cm->advanceDay();
                    }
                } else if (btn.action == "Skip Summer") {
                    CareerManager* cm = m_gameManager->getCareerManager();
                    while (cm->isSummerBreak()) {
                        cm->advanceDay();
                    }
                } else if (btn.action == "Upgrades") {
                    m_gameManager->changeScreen(std::make_shared<UpgradeScreen>());
                } else if (btn.action == "League Table") {
                    m_gameManager->changeScreen(std::make_shared<LeagueTableScreen>());
                } else if (btn.action == "Debug: Skip Match") {
                    if (m_gameManager->getCareerManager()->getDayType() == CalendarDayType::Match || m_gameManager->getCareerManager()->hasInternationalMatchToday()) {
                        Player* p = m_gameManager->getPlayer();
                        if (p->suspensionMatches > 0) p->suspensionMatches--;
                        Club* opp = nullptr;
                        Club* playerClub = p->currentClub;
                        bool isHomeMatch = true;
                        
                        if (m_gameManager->getCareerManager()->hasInternationalMatchToday()) {
                            opp = m_gameManager->getCareerManager()->getInternationalOpponent();
                            isHomeMatch = m_gameManager->getCareerManager()->isHomeInternationalMatch();
                            // Player's national team plays, so we pass nationality as club name.
                            // We will handle this nicely by passing a dummy club or finding the national team.
                            const League* nats = m_gameManager->getDatabase().getNationalTeams();
                            if (nats) {
                                for (auto& c : nats->clubs) {
                                    if (c.name == p->nationality) playerClub = const_cast<Club*>(&c);
                                }
                            }
                        } else if (m_gameManager->getCareerManager()->hasEuropeanMatchToday()) {
                            opp = m_gameManager->getCareerManager()->getTodayOpponent();
                            isHomeMatch = m_gameManager->getCareerManager()->isHomeMatchToday();
                        } else {
                            const League* lg = nullptr;
                            for (const auto& l : m_gameManager->getDatabase().getLeagues()) {
                                for (const auto& c : l.clubs) {
                                    if (c.name == p->currentClub->name) {
                                        lg = &l;
                                        break;
                                    }
                                }
                            }
                            
                            if (lg) {
                                int n = static_cast<int>(lg->clubs.size());
                                int r = p->weeksPlayed % (n - 1);
                                
                                int pIndex = -1;
                                for (int i = 0; i < n; ++i) {
                                    if (lg->clubs[i].name == p->currentClub->name) {
                                        pIndex = i;
                                        break;
                                    }
                                }
                                
                                auto rotate = [n, r](int x) {
                                    if (x == 0) return 0;
                                    return 1 + (x - 1 + r) % (n - 1);
                                };
                                
                                for (int i = 0; i < n / 2; ++i) {
                                    int t1 = (i == 0) ? 0 : rotate(i);
                                    int t2 = rotate(n - 1 - i);
                                    
                                    if (t1 == pIndex) {
                                        opp = m_gameManager->getDatabase().getClub(lg->name, lg->clubs[t2].name);
                                        break;
                                    } else if (t2 == pIndex) {
                                        opp = m_gameManager->getDatabase().getClub(lg->name, lg->clubs[t1].name);
                                        isHomeMatch = false;
                                        break;
                                    }
                                }
                            }
                        }
                        
                        std::shared_ptr<MatchEngine> engine = std::make_shared<MatchEngine>(playerClub, opp, isHomeMatch, p);
                        
                        while (engine->getState() != MatchState::Finished) {
                            if (engine->getState() == MatchState::Simulating) {
                                engine->updateMinute();
                            } else if (engine->getState() == MatchState::MinigameTriggered) {
                                engine->processMinigameResult(rand() % 2 == 0); // 50% win rate for auto-sim
                            }
                            
                            while (engine->hasLogs()) {
                                engine->commitEvent(engine->popRecentLog());
                            }
                        }
                        m_gameManager->changeScreen(std::make_shared<MatchStatsScreen>(engine));
                    }
                } else if (btn.action == "Debug: Skip Training") {
                    if (m_gameManager->getCareerManager()->getDayType() == CalendarDayType::Training) {
                        Player* p = m_gameManager->getPlayer();
                        p->experience += 50;
                        m_gameManager->getCareerManager()->advanceDay();
                    }
                } else if (btn.action == "Debug: Skip Season") {
                    m_gameManager->getCareerManager()->skipSeason();
                } else if (btn.action == "Debug: Skip Season") {
                    m_gameManager->getCareerManager()->skipSeason();
                }
            }
        }
    }
    
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2i pixelPos(event.mouseMove.x, event.mouseMove.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        for (auto& btn : m_buttons) {
            btn.isHovered = btn.rect.getGlobalBounds().contains(mousePos);
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
        stats += "Nationality: " + p->nationality + "\n";
        stats += "Shooting: " + std::to_string(p->shooting) + "\n";
        stats += "Passing: " + std::to_string(p->passing) + "\n";
        stats += "Morale: " + std::to_string(p->morale) + "\n";
        stats += "Energy: " + std::to_string(p->energy) + "%\n";
        stats += "XP: " + std::to_string(p->experience) + "\n";
        stats += "Money: $" + std::to_string(p->money) + "\n";
        stats += "Salary: $" + std::to_string(p->salary) + "/w\n";
        m_playerStatsText.setString(stats);
        
        if (cm && cm->isSummerBreak()) {
            if (cm->hasInternationalMatchToday()) {
                m_buttons[0].text.setString("Play Int. Match");
                m_buttons[0].action = "Advance Day";
                m_buttons[0].baseColor = sf::Color(50, 150, 50);
            } else if (!cm->hasRemainingInternationalMatches()) {
                m_buttons[0].text.setString("Proceed to Club Season");
                m_buttons[0].action = "Skip Summer";
                m_buttons[0].baseColor = sf::Color(100, 100, 150);
            } else {
                m_buttons[0].text.setString("Simulate Day");
                m_buttons[0].action = "Advance Day";
                m_buttons[0].baseColor = UITheme::ButtonNormal;
            }
        } else if (p->injuredDays > 0) {
            m_buttons[0].text.setString("Recovery (Rest)");
            m_buttons[0].action = "Recovery";
            m_buttons[0].baseColor = sf::Color(150, 50, 50);
        } else {
            m_buttons[0].text.setString("Advance Day");
            m_buttons[0].action = "Advance Day";
            m_buttons[0].baseColor = UITheme::ButtonNormal;
        }

        // Handle Transfer Window
        if (p->weeksPlayed > 0 && p->weeksPlayed % 15 == 0) {
            bool hasTransferBtn = false;
            for (auto& b : m_buttons) if (b.action == "View Offers") hasTransferBtn = true;
            
            if (!hasTransferBtn) {
                Button btnOff;
                btnOff.rect.setSize(sf::Vector2f(200.f, 40.f));
                btnOff.rect.setPosition(450.f, 300.f);
                btnOff.baseColor = sf::Color(200, 150, 50);
                
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
        if (cm->isSummerBreak()) {
            std::string cal = "SUMMER BREAK\n";
            cal += p->isCalledUp ? "You have been called up\nfor National Team!" : "You are resting\nthis summer.";
            m_calendarText.setFillColor(sf::Color(255, 165, 0)); // Orange
            m_calendarText.setString(cal);
        } else {
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
}

void CareerHubScreen::draw(sf::RenderWindow& window) {
    UITheme::drawGradientBackground(window);
    window.draw(m_titleText);
    window.draw(m_playerStatsText);
    window.draw(m_calendarText);
    
    for (const auto& btn : m_buttons) {
        sf::RectangleShape renderRect = btn.rect;
        renderRect.setFillColor(btn.isHovered ? sf::Color(std::min(255, btn.baseColor.r + 50), std::min(255, btn.baseColor.g + 50), std::min(255, btn.baseColor.b + 50)) : btn.baseColor);
        window.draw(renderRect);
        window.draw(btn.text);
    }
    
    sf::RectangleShape settingsRenderRect = m_btnSettings.rect;
    settingsRenderRect.setFillColor(m_btnSettings.isHovered ? sf::Color(std::min(255, m_btnSettings.baseColor.r + 50), std::min(255, m_btnSettings.baseColor.g + 50), std::min(255, m_btnSettings.baseColor.b + 50)) : m_btnSettings.baseColor);
    window.draw(settingsRenderRect);
    window.draw(m_btnSettings.text);
}
