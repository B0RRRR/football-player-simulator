#include "NewCareerScreen.h"
#include "CareerHubScreen.h"
#include "CareerManager.h"
#include "MatchScreen.h"
#include "MenuScreen.h"
#include "GameManager.h"
#include "AssetManager.h"
#include <iostream>
#include <random>

NewCareerScreen::NewCareerScreen() : m_state(SetupState::InputName), m_playerName(""), m_selectedPosition(3) {
}

void NewCareerScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    m_titleText.setFont(font);
    m_titleText.setCharacterSize(40);
    m_titleText.setFillColor(sf::Color::White);
    m_titleText.setPosition(100.f, 50.f);
    
    m_infoText.setFont(font);
    m_infoText.setCharacterSize(24);
    m_infoText.setFillColor(sf::Color::Yellow);
    m_infoText.setPosition(100.f, 120.f);

    m_inputText.setFont(font);
    m_inputText.setCharacterSize(30);
    m_inputText.setFillColor(sf::Color::Cyan);
    m_inputText.setPosition(100.f, 200.f);
    
    rebuildButtons();
}

void NewCareerScreen::rebuildButtons() {
    m_buttons.clear();
    auto& font = AssetManager::get().getFont("MainFont");
    
    std::vector<std::string> labels;
    std::vector<std::string> actions;
    float startY = 180.f;

    if (m_state == SetupState::InputName) {
        m_titleText.setString("Step 1: Enter Your Name");
        m_infoText.setString("Type your name and press Enter to continue...");
        // Button is created only for generic next, but we use keyboard Enter.
    } else if (m_state == SetupState::SelectNationality) {
        m_titleText.setString("Step 2: Choose Nationality");
        m_infoText.setString("Select your country (European Only for now):");
        
        std::vector<std::string> euroNames = {
            "France", "Spain", "England", "Netherlands", "Portugal", "Belgium",
            "Germany", "Croatia", "Italy", "Switzerland", "Denmark", "Austria",
            "Norway", "Turkey", "Russia", "Sweden"
        };
        for (const auto& name : euroNames) {
            labels.push_back(name);
            actions.push_back("NAT_" + name);
        }
    } else if (m_state == SetupState::SelectPosition) {
        m_titleText.setString("Step 3: Choose Position");
        m_infoText.setString("Select your role on the pitch:");
        labels = {"Goalkeeper", "Defender", "Midfielder", "Forward"};
        actions = {"POS_0", "POS_1", "POS_2", "POS_3"};
    } else if (m_state == SetupState::SelectLeague) {
        m_titleText.setString("Step 3: Choose League");
        m_infoText.setString("Where do you want to start?");
        
        const auto& leagues = m_gameManager->getDatabase().getLeagues();
        for (const auto& l : leagues) {
            labels.push_back(l.name);
            actions.push_back("LEAGUE_" + l.name);
        }
    } else if (m_state == SetupState::SelectClub) {
        m_titleText.setString("Step 4: Choose League");
        m_infoText.setString("Pick a team to sign your first contract:");
        
        auto leagues = m_gameManager->getDatabase().getLeagues();
        std::vector<const Club*> weakClubs;
        for (const auto& league : leagues) {
            for (const auto& club : league.clubs) {
                if (club.strength <= 75) {
                    weakClubs.push_back(&club);
                }
            }
        }
        
        if (weakClubs.size() > 6) {
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(weakClubs.begin(), weakClubs.end(), g);
            weakClubs.resize(6);
        }
        
        for (const auto* c : weakClubs) {
            labels.push_back(c->name + " (STR: " + std::to_string(c->strength) + ")");
            actions.push_back("CLUB_" + c->name);
        }
    }
    
    for (size_t i = 0; i < labels.size(); ++i) {
        Button btn;
        btn.rect.setSize(sf::Vector2f(400.f, 30.f));
        
        // Wrap to two columns if more than 10 buttons (like nationalities)
        if (labels.size() > 10) {
            btn.rect.setSize(sf::Vector2f(300.f, 30.f));
            if (i < labels.size() / 2) {
                btn.rect.setPosition(100.f, startY + i * 35.f);
            } else {
                btn.rect.setPosition(420.f, startY + (i - labels.size() / 2) * 35.f);
            }
        } else {
            btn.rect.setPosition(100.f, startY + i * 50.f);
        }
        
        btn.rect.setFillColor(sf::Color(100, 100, 100));
        
        btn.text.setFont(font);
        btn.text.setString(sf::String::fromUtf8(labels[i].begin(), labels[i].end()));
        btn.text.setCharacterSize(18);
        btn.text.setFillColor(sf::Color::White);
        
        sf::FloatRect textRect = btn.text.getLocalBounds();
        btn.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
        btn.text.setPosition(btn.rect.getPosition().x + btn.rect.getSize().x/2.0f,
                             btn.rect.getPosition().y + btn.rect.getSize().y/2.0f);
        
        btn.action = actions[i];
        m_buttons.push_back(btn);
    }
    
    // Always add a Back/Cancel button at the bottom
    Button btnBack;
    btnBack.rect.setSize(sf::Vector2f(200.f, 40.f));
    btnBack.rect.setPosition(100.f, 550.f);
    btnBack.rect.setFillColor(sf::Color(150, 50, 50));
    btnBack.text.setFont(font);
    btnBack.text.setString("Cancel");
    btnBack.text.setCharacterSize(20);
    btnBack.text.setFillColor(sf::Color::White);
    sf::FloatRect textRect = btnBack.text.getLocalBounds();
    btnBack.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
    btnBack.text.setPosition(btnBack.rect.getPosition().x + btnBack.rect.getSize().x/2.0f,
                             btnBack.rect.getPosition().y + btnBack.rect.getSize().y/2.0f);
    btnBack.action = "Cancel";
    m_buttons.push_back(btnBack);
}

void NewCareerScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (m_state == SetupState::InputName) {
        if (event.type == sf::Event::TextEntered) {
            if (event.text.unicode == '\b' && !m_playerName.empty()) {
                m_playerName.pop_back();
            } else if (event.text.unicode >= 32 && event.text.unicode < 128 && m_playerName.size() < 20) {
                m_playerName += static_cast<char>(event.text.unicode);
            }
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
            if (!m_playerName.empty()) {
                m_state = SetupState::SelectNationality;
                rebuildButtons();
            }
        }
    }
    
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
        
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                if (btn.action == "Cancel") {
                    m_gameManager->changeScreen(std::make_shared<MenuScreen>());
                    return;
                }
                
                if (m_state == SetupState::SelectNationality && btn.action.find("NAT_") == 0) {
                    m_selectedNationality = btn.action.substr(4);
                    m_state = SetupState::SelectPosition;
                    rebuildButtons();
                } else if (m_state == SetupState::SelectPosition && btn.action.find("POS_") == 0) {
                    m_selectedPosition = std::stoi(btn.action.substr(4));
                    m_state = SetupState::SelectClub;
                    rebuildButtons();
                } else if (m_state == SetupState::SelectClub && btn.action.find("CLUB_") == 0) {
                    m_selectedClub = btn.action.substr(5);
                    
                    // Finalize creation
                    Player* p = m_gameManager->getPlayer();
                    p->name = m_playerName;
                    p->nationality = m_selectedNationality;
                    p->position = static_cast<PlayerPosition>(m_selectedPosition);
                    // Find the club object
                    const Club* selectedClubObj = nullptr;
                    for (const auto& l : m_gameManager->getDatabase().getLeagues()) {
                        for (const auto& c : l.clubs) {
                            if (c.name == m_selectedClub) {
                                selectedClubObj = &c;
                                break;
                            }
                        }
                    }

                    p->currentClub = const_cast<Club*>(selectedClubObj);
                    p->experience = 0;
                    p->goals = 0;
                    p->assists = 0;
                    
                    // Initialize career
                    m_gameManager->getCareerManager()->resetCareer();
                    
                    // Proceed to Career Hub
                    m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
                    return;
                }
            }
        }
    }
    
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos(static_cast<float>(event.mouseMove.x), static_cast<float>(event.mouseMove.y));
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                btn.rect.setFillColor(sf::Color(150, 150, 150));
            } else {
                if (btn.action == "Cancel") btn.rect.setFillColor(sf::Color(150, 50, 50));
                else btn.rect.setFillColor(sf::Color(100, 100, 100));
            }
        }
    }
}

void NewCareerScreen::update(sf::Time deltaTime) {
    if (m_state == SetupState::InputName) {
        m_inputText.setString(m_playerName + "_");
    } else {
        m_inputText.setString("");
    }
}

void NewCareerScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(20, 20, 40));
    window.draw(m_titleText);
    window.draw(m_infoText);
    
    if (m_state == SetupState::InputName) {
        window.draw(m_inputText);
    }
    
    for (const auto& btn : m_buttons) {
        window.draw(btn.rect);
        window.draw(btn.text);
    }
}
