#include "EuropeanCupScreen.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "Player.h"
#include <sstream>

EuropeanCupScreen::EuropeanCupScreen() : m_currentView(0) {
}

void EuropeanCupScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    m_titleText.setFont(font);
    m_titleText.setCharacterSize(36);
    m_titleText.setFillColor(sf::Color::Yellow);
    m_titleText.setPosition(50.f, 20.f);
    m_titleText.setString("Tournaments");
    
    m_statusText.setFont(font);
    m_statusText.setCharacterSize(20);
    m_statusText.setFillColor(sf::Color::White);
    m_statusText.setPosition(50.f, 70.f);
    
    auto createBtn = [&](const std::string& label, float x, float y, const std::string& action) {
        Button btn;
        btn.rect.setSize(sf::Vector2f(200.f, 40.f));
        btn.rect.setPosition(x, y);
        btn.rect.setFillColor(sf::Color(70, 70, 70));
        
        btn.text.setFont(font);
        btn.text.setString(label);
        btn.text.setCharacterSize(16);
        btn.text.setFillColor(sf::Color::White);
        
        sf::FloatRect tr = btn.text.getLocalBounds();
        btn.text.setOrigin(tr.left + tr.width/2.0f, tr.top + tr.height/2.0f);
        btn.text.setPosition(x + 100.f, y + 20.f);
        
        btn.action = action;
        m_buttons.push_back(btn);
    };
    
    createBtn("Champions League", 50.f, 140.f, "CL");
    createBtn("Europa League", 260.f, 140.f, "EL");
    createBtn("Int. Tournament", 470.f, 140.f, "INT");
    createBtn("Back", 600.f, 520.f, "BACK");
    
    updateBracketVisuals();
}

void EuropeanCupScreen::updateBracketVisuals() {
    m_bracketTexts.clear();
    auto& font = AssetManager::get().getFont("MainFont");
    
    Tournament& t = m_currentView == 0 ? m_gameManager->getDatabase().getChampionsLeague() : 
                    (m_currentView == 1 ? m_gameManager->getDatabase().getEuropaLeague() :
                    (m_gameManager->getCareerManager()->getYear() % 2 == 0 ? m_gameManager->getDatabase().getEuroCup() : m_gameManager->getDatabase().getWorldCup()));
    
    Player* p = m_gameManager->getPlayer();
    std::string playerStatus = "You are not participating in this tournament.";
    bool found = false;
    
    float startY = 200.f;
    float startX = 50.f;
    for (size_t i = 0; i < t.rounds.size(); ++i) {
        sf::Text roundTitle;
        roundTitle.setFont(font);
        roundTitle.setCharacterSize(20);
        roundTitle.setFillColor(sf::Color(150, 150, 255));
        roundTitle.setPosition(startX, startY);
        roundTitle.setString(t.rounds[i].name);
        m_bracketTexts.push_back(roundTitle);
        
        float y = startY + 30.f;
        for (const auto& m : t.rounds[i].matches) {
            if (m_currentView < 2) {
                if (m.home && p && p->currentClub && m.home->name == p->currentClub->name) found = true;
                if (m.away && p && p->currentClub && m.away->name == p->currentClub->name) found = true;
            } else {
                if (m.home && p && m.home->name == p->nationality && p->isCalledUp) found = true;
                if (m.away && p && m.away->name == p->nationality && p->isCalledUp) found = true;
            }
            
            std::stringstream ss;
            if (m.winner == m.home) ss << "[*] ";
            ss << (m.home ? m.home->name : "TBD") << " ";
            
            if (m.isFinal) {
                if (m.leg1Played) {
                    ss << m.homeGoalsLeg1 << " - " << m.awayGoalsLeg1;
                    if (m.homeGoalsLeg1 == m.awayGoalsLeg1) {
                        ss << " (" << m.homePenalties << "-" << m.awayPenalties << " p.)";
                    }
                } else {
                    ss << "vs";
                }
            } else {
                if (m.leg2Played) {
                    int agg1 = m.homeGoalsLeg1 + m.awayGoalsLeg2;
                    int agg2 = m.awayGoalsLeg1 + m.homeGoalsLeg2;
                    ss << agg1 << " - " << agg2 << " (Agg)";
                    if (agg1 == agg2) {
                        ss << " (" << m.homePenalties << "-" << m.awayPenalties << " p.)";
                    }
                } else if (m.leg1Played) {
                    ss << m.homeGoalsLeg1 << " - " << m.awayGoalsLeg1 << " (Leg 1)";
                } else {
                    ss << "vs";
                }
            }
            
            ss << " " << (m.away ? m.away->name : "TBD");
            if (m.winner == m.away) ss << " [*]";
            
            sf::Text matchText;
            matchText.setFont(font);
            matchText.setCharacterSize(16);
            matchText.setFillColor(sf::Color::White);
            matchText.setPosition(startX + 20.f, y);
            matchText.setString(ss.str());
            m_bracketTexts.push_back(matchText);
            
            y += 25.f;
        }
        
        startY = y + 15.f;
        // Explicitly wrap after Round of 32 and Round of 16 to keep it clean (3 columns)
        if ((i == 0 || i == 1) && t.rounds.size() > 1) {
            startY = 200.f;
            startX += 380.f; 
        }
    }
    
    if (found) {
        playerStatus = "You are participating in this tournament!";
    }
    
    if (t.rounds.empty()) {
        playerStatus = "Tournament has not been generated yet.";
    }
    
    if (t.isFinished && t.winner) {
        playerStatus += "\nTournament Winner: " + t.winner->name;
    }
    
    m_statusText.setString(playerStatus);
}

void EuropeanCupScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed) {
        sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                if (btn.action == "CL") {
                    m_currentView = 0;
                    updateBracketVisuals();
                } else if (btn.action == "EL") {
                    m_currentView = 1;
                    updateBracketVisuals();
                } else if (btn.action == "INT") {
                    m_currentView = 2;
                    updateBracketVisuals();
                } else if (btn.action == "BACK") {
                    m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
                }
            }
        }
    }
}

void EuropeanCupScreen::update(sf::Time deltaTime) {
}

void EuropeanCupScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(20, 20, 30));
    window.draw(m_titleText);
    window.draw(m_statusText);
    
    for (const auto& btn : m_buttons) {
        sf::RectangleShape r = btn.rect;
        if (btn.action == "CL" && m_currentView == 0) r.setFillColor(sf::Color(100, 150, 100));
        if (btn.action == "EL" && m_currentView == 1) r.setFillColor(sf::Color(100, 150, 100));
        if (btn.action == "INT" && m_currentView == 2) r.setFillColor(sf::Color(100, 150, 100));
        window.draw(r);
        window.draw(btn.text);
    }
    
    for (const auto& t : m_bracketTexts) {
        window.draw(t);
    }
}
