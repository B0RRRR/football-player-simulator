#include "UITheme.h"
#include "EuropeanCupScreen.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "Player.h"
#include <sstream>

EuropeanCupScreen::EuropeanCupScreen() : m_currentView(0), m_selectedYear(0), m_maxYear(0) {
}

void EuropeanCupScreen::init() {
    m_maxYear = m_gameManager->getCareerManager()->getYear();
    m_selectedYear = m_maxYear;
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
        btn.rect.setFillColor(UITheme::ButtonNormal);
        
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
    createBtn("Int. Tournament", 470.f, 140.f, "INT");
    createBtn("Back", 600.f, 520.f, "BACK");
    
    createBtn("<", 650.f, 25.f, "PREV");
    createBtn(">", 700.f, 25.f, "NEXT");
    
    // adjust sizes of PREV/NEXT
    for (auto& btn : m_buttons) {
        if (btn.action == "PREV" || btn.action == "NEXT") {
            btn.rect.setSize(sf::Vector2f(40.f, 30.f));
            btn.text.setPosition(btn.rect.getPosition().x + 20.f, btn.rect.getPosition().y + 15.f);
        }
    }
    
    updateBracketVisuals();
}

void EuropeanCupScreen::updateBracketVisuals() {
    m_bracketTexts.clear();
    auto& font = AssetManager::get().getFont("MainFont");
    
    Database& db = m_gameManager->getDatabase();
    Tournament t;
    bool isHistory = (m_selectedYear != m_maxYear);
    
    if (m_currentView == 0) {
        if (isHistory && db.getChampionsLeagueHistory().count(m_selectedYear))
            t = db.getChampionsLeagueHistory().at(m_selectedYear);
        else t = db.getChampionsLeague();
    } else if (m_currentView == 1) {
        if (isHistory && db.getEuropaLeagueHistory().count(m_selectedYear))
            t = db.getEuropaLeagueHistory().at(m_selectedYear);
        else t = db.getEuropaLeague();
    } else {
        if (m_selectedYear % 2 == 0) {
            if (isHistory && db.getEuroCupHistory().count(m_selectedYear))
                t = db.getEuroCupHistory().at(m_selectedYear);
            else t = db.getEuroCup();
        } else {
            if (isHistory && db.getWorldCupHistory().count(m_selectedYear))
                t = db.getWorldCupHistory().at(m_selectedYear);
            else t = db.getWorldCup();
        }
    }
    
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
    
    if (t.rounds.empty()) {
        playerStatus = "Tournament has not been generated yet.";
    } else {
        if (found && !isHistory) {
            playerStatus = "You are participating in this tournament.";
        } else if (isHistory) {
            playerStatus = "Historical view.";
        }
        
        if (t.isFinished && t.winner) {
            playerStatus += "\nTournament Winner: " + t.winner->name;
        }
    }
    
    std::string viewName = m_currentView == 0 ? "Champions League" : (m_currentView == 1 ? "Europa League" : (m_selectedYear % 2 == 0 ? "Euro Cup" : "World Cup"));
    m_statusText.setString(viewName + " (" + std::to_string(m_selectedYear) + ")\n" + playerStatus);
}

void EuropeanCupScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
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
                } else if (btn.action == "PREV") {
                    if (m_selectedYear > 2024) { // Assuming 2024 is start year
                        m_selectedYear--;
                        updateBracketVisuals();
                    }
                } else if (btn.action == "NEXT") {
                    if (m_selectedYear < m_maxYear) {
                        m_selectedYear++;
                        updateBracketVisuals();
                    }
                }
            }
        }
    }
}

void EuropeanCupScreen::update(sf::Time deltaTime) {
}

void EuropeanCupScreen::draw(sf::RenderWindow& window) {
    UITheme::drawGradientBackground(window);
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
