#include "UITheme.h"
#include "LeagueTableScreen.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "CareerManager.h"
#include "Player.h"
#include "AssetManager.h"
#include "SeasonEndScreen.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

LeagueTableScreen::LeagueTableScreen() {
}

void LeagueTableScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    m_titleText.setFont(font);
    m_titleText.setCharacterSize(30);
    m_titleText.setFillColor(sf::Color::White);
    m_titleText.setPosition(50.f, 20.f);
    m_titleText.setString("League Table");
    
    m_backButton.rect.setSize(sf::Vector2f(200.f, 40.f));
    m_backButton.rect.setPosition(50.f, 530.f);
    m_backButton.rect.setFillColor(sf::Color(150, 50, 50));
    
    m_backButton.text.setFont(font);
    m_backButton.text.setString("Back to Hub");
    m_backButton.text.setCharacterSize(20);
    m_backButton.text.setFillColor(sf::Color::White);
    sf::FloatRect textRect = m_backButton.text.getLocalBounds();
    m_backButton.text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
    m_backButton.text.setPosition(m_backButton.rect.getPosition().x + m_backButton.rect.getSize().x/2.0f,
                                  m_backButton.rect.getPosition().y + m_backButton.rect.getSize().y/2.0f);
    m_backButton.action = "Back";
    
    m_viewedYear = m_gameManager->getCareerManager()->getYear();
    
    // Prev year button
    m_prevYearBtn.rect.setSize(sf::Vector2f(40.f, 40.f));
    m_prevYearBtn.rect.setPosition(650.f, 15.f);
    m_prevYearBtn.rect.setFillColor(UITheme::ButtonNormal);
    m_prevYearBtn.text.setFont(font);
    m_prevYearBtn.text.setString("<");
    m_prevYearBtn.text.setCharacterSize(20);
    m_prevYearBtn.text.setFillColor(sf::Color::White);
    m_prevYearBtn.text.setPosition(660.f, 20.f);
    m_prevYearBtn.action = "Prev";
    
    // Next year button
    m_nextYearBtn.rect.setSize(sf::Vector2f(40.f, 40.f));
    m_nextYearBtn.rect.setPosition(700.f, 15.f);
    m_nextYearBtn.rect.setFillColor(UITheme::ButtonNormal);
    m_nextYearBtn.text.setFont(font);
    m_nextYearBtn.text.setString(">");
    m_nextYearBtn.text.setCharacterSize(20);
    m_nextYearBtn.text.setFillColor(sf::Color::White);
}

void LeagueTableScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        
        if (m_backButton.rect.getGlobalBounds().contains(mousePos)) {
            if (m_gameManager->getPlayer()->weeksPlayed >= m_gameManager->getCareerManager()->getSeasonLength()) {
                m_gameManager->changeScreen(std::make_shared<SeasonEndScreen>());
            } else {
                m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
            }
        } else if (m_prevYearBtn.rect.getGlobalBounds().contains(mousePos)) {
            m_viewedYear--;
        } else if (m_nextYearBtn.rect.getGlobalBounds().contains(mousePos)) {
            m_viewedYear++;
        }
        
        for (const auto& btn : m_leagueButtons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                m_viewedLeagueName = btn.action;
                break;
            }
        }
    }
    
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2i pixelPos(event.mouseMove.x, event.mouseMove.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        if (m_backButton.rect.getGlobalBounds().contains(mousePos)) {
            m_backButton.rect.setFillColor(sf::Color(200, 50, 50));
        } else {
            m_backButton.rect.setFillColor(sf::Color(150, 50, 50));
        }
    }
}

void LeagueTableScreen::update(sf::Time deltaTime) {
    m_tableRows.clear();
    m_tableLogos.clear();
    auto& font = AssetManager::get().getFont("MainFont");
    
    Player* p = m_gameManager->getPlayer();
    if (!p || !p->currentClub) return;
    
    const League* currentLeague = nullptr;
    const std::vector<League>* sourceLeagues = nullptr;
    
    int currentYear = m_gameManager->getCareerManager()->getYear();
    if (m_viewedYear > currentYear) m_viewedYear = currentYear;
    if (m_viewedYear < 2024) m_viewedYear = 2024; // Assuming game starts at 2024
    
    if (m_viewedYear == currentYear) {
        sourceLeagues = &m_gameManager->getDatabase().getLeagues();
    } else {
        auto& history = m_gameManager->getDatabase().getLeagueHistory();
        if (history.find(m_viewedYear) != history.end()) {
            sourceLeagues = &history.at(m_viewedYear);
        }
    }
    
    if (sourceLeagues && !sourceLeagues->empty()) {
        if (m_viewedLeagueName.empty()) {
            // Find player's league
            for (const auto& l : *sourceLeagues) {
                for (const auto& c : l.clubs) {
                    if (c.name == p->currentClub->name) {
                        m_viewedLeagueName = l.name;
                        break;
                    }
                }
                if (!m_viewedLeagueName.empty()) break;
            }
            if (m_viewedLeagueName.empty()) m_viewedLeagueName = (*sourceLeagues)[0].name;
        }
        
        // Find the league by name
        for (const auto& l : *sourceLeagues) {
            if (l.name == m_viewedLeagueName) {
                currentLeague = &l;
                break;
            }
        }
        
        // Fallback
        if (!currentLeague) currentLeague = &(*sourceLeagues)[0];
        m_viewedLeagueName = currentLeague->name;
    } else {
        return;
    }
    
    // Create league selector buttons
    m_leagueButtons.clear();
    if (sourceLeagues) {
        float btnX = 750.f;
        float btnY = 100.f;
        for (const auto& l : *sourceLeagues) {
            Button btn;
            btn.text.setFont(font);
            btn.text.setString(l.name);
            btn.text.setCharacterSize(14);
            sf::FloatRect textRect = btn.text.getLocalBounds();
            
            btn.rect.setSize(sf::Vector2f(textRect.width + 20.f, 25.f));
            btn.rect.setPosition(btnX, btnY);
            if (l.name == m_viewedLeagueName) {
                btn.rect.setFillColor(sf::Color(100, 100, 200)); // Highlight
            } else {
                btn.rect.setFillColor(UITheme::ButtonNormal);
            }
            
            btn.text.setPosition(btnX + 10.f, btnY + 4.f);
            btn.text.setFillColor(sf::Color::White);
            btn.action = l.name;
            
            m_leagueButtons.push_back(btn);
            btnY += 35.f; // Move down for the next button
        }
    }
    
    std::string titleStr = currentLeague->name + " (" + std::to_string(m_viewedYear) + "/" + std::to_string(m_viewedYear + 1) + ")";
    m_titleText.setString(sf::String::fromUtf8(titleStr.begin(), titleStr.end()));
    
    // Sort clubs
    std::vector<Club> sortedClubs = currentLeague->clubs;
    std::sort(sortedClubs.begin(), sortedClubs.end(), [](const Club& a, const Club& b) {
        if (a.points != b.points) return a.points > b.points;
        int gdA = a.goalsFor - a.goalsAgainst;
        int gdB = b.goalsFor - b.goalsAgainst;
        if (gdA != gdB) return gdA > gdB;
        return a.goalsFor > b.goalsFor;
    });
    
    // Header
    auto addColumn = [&](const std::string& str, float x, float y, sf::Color color) {
        sf::Text t;
        t.setFont(font);
        t.setCharacterSize(16);
        t.setFillColor(color);
        t.setPosition(x, y);
        t.setString(sf::String::fromUtf8(str.begin(), str.end()));
        m_tableRows.push_back(t);
    };

    sf::Color headC = sf::Color::Cyan;
    addColumn("POS", 50.f, 80.f, headC);
    addColumn("CLUB", 100.f, 80.f, headC);
    addColumn("PTS", 350.f, 80.f, headC);
    addColumn("W", 400.f, 80.f, headC);
    addColumn("D", 450.f, 80.f, headC);
    addColumn("L", 500.f, 80.f, headC);
    addColumn("GF", 550.f, 80.f, headC);
    addColumn("GA", 600.f, 80.f, headC);
    addColumn("GD", 650.f, 80.f, headC);
    
    float startY = 110.f;
    float rowHeight = 20.f;
    for (size_t i = 0; i < sortedClubs.size(); ++i) {
        const auto& c = sortedClubs[i];
        sf::Color cColor = (c.name == p->currentClub->name) ? sf::Color::Yellow : sf::Color::White;
        float y = startY + i * rowHeight;
        
        addColumn(std::to_string(i+1), 50.f, y, cColor);
        
        // Draw Logo
        sf::Sprite logo;
        bool isNat = (currentLeague->name == "National Teams");
        logo.setTexture(AssetManager::get().getTexture(c.name, isNat));
        logo.setPosition(80.f, y);
        float scaleX = 20.f / logo.getTexture()->getSize().x;
        float scaleY = 20.f / logo.getTexture()->getSize().y;
        logo.setScale(scaleX, scaleY);
        m_tableLogos.push_back(logo);
        
        addColumn(c.name, 110.f, y, cColor);
        addColumn(std::to_string(c.points), 350.f, y, cColor);
        addColumn(std::to_string(c.wins), 400.f, y, cColor);
        addColumn(std::to_string(c.draws), 450.f, y, cColor);
        addColumn(std::to_string(c.losses), 500.f, y, cColor);
        addColumn(std::to_string(c.goalsFor), 550.f, y, cColor);
        addColumn(std::to_string(c.goalsAgainst), 600.f, y, cColor);
        addColumn(std::to_string(c.goalsFor - c.goalsAgainst), 650.f, y, cColor);
    }
}

void LeagueTableScreen::draw(sf::RenderWindow& window) {
    UITheme::drawGradientBackground(window);
    window.draw(m_titleText);
    
    for (const auto& row : m_tableRows) {
        window.draw(row);
    }
    
    for (const auto& logo : m_tableLogos) {
        window.draw(logo);
    }
    
    for (const auto& logo : m_tableLogos) {
        window.draw(logo);
    }
    
    window.draw(m_backButton.rect);
    window.draw(m_backButton.text);
    
    window.draw(m_prevYearBtn.rect);
    window.draw(m_prevYearBtn.text);
    
    window.draw(m_nextYearBtn.rect);
    window.draw(m_nextYearBtn.text);
    
    for (const auto& btn : m_leagueButtons) {
        window.draw(btn.rect);
        window.draw(btn.text);
    }
}
