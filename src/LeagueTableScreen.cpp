#include "LeagueTableScreen.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "Player.h"
#include "Database.h"
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
}

void LeagueTableScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
        
        if (m_backButton.rect.getGlobalBounds().contains(mousePos)) {
            m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
        }
    }
    
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos(event.mouseMove.x, event.mouseMove.y);
        if (m_backButton.rect.getGlobalBounds().contains(mousePos)) {
            m_backButton.rect.setFillColor(sf::Color(200, 50, 50));
        } else {
            m_backButton.rect.setFillColor(sf::Color(150, 50, 50));
        }
    }
}

void LeagueTableScreen::update(sf::Time deltaTime) {
    m_tableRows.clear();
    auto& font = AssetManager::get().getFont("MainFont");
    
    Player* p = m_gameManager->getPlayer();
    if (!p || !p->currentClub) return;
    
    const League* currentLeague = nullptr;
    for (const auto& l : m_gameManager->getDatabase().getLeagues()) {
        for (const auto& c : l.clubs) {
            if (c.name == p->currentClub->name) {
                currentLeague = &l;
                break;
            }
        }
    }
    
    if (!currentLeague) return;
    
    m_titleText.setString(currentLeague->name + " Standings");
    
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
    sf::Text header;
    header.setFont(font);
    header.setCharacterSize(20);
    header.setFillColor(sf::Color::Cyan);
    header.setPosition(50.f, 80.f);
    header.setString("POS  CLUB                         PTS   W   D   L   GF  GA  GD");
    m_tableRows.push_back(header);
    
    float startY = 120.f;
    for (size_t i = 0; i < sortedClubs.size(); ++i) {
        const auto& c = sortedClubs[i];
        
        std::stringstream ss;
        ss << std::left << std::setw(4) << (i + 1)
           << std::setw(28) << c.name
           << std::setw(6) << c.points
           << std::setw(4) << c.wins
           << std::setw(4) << c.draws
           << std::setw(4) << c.losses
           << std::setw(4) << c.goalsFor
           << std::setw(4) << c.goalsAgainst
           << std::setw(4) << (c.goalsFor - c.goalsAgainst);
           
        sf::Text row;
        row.setFont(font);
        row.setCharacterSize(20);
        if (c.name == p->currentClub->name) {
            row.setFillColor(sf::Color::Yellow);
        } else {
            row.setFillColor(sf::Color::White);
        }
        row.setPosition(50.f, startY + i * 35.f);
        row.setString(ss.str());
        m_tableRows.push_back(row);
    }
}

void LeagueTableScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(20, 20, 40));
    window.draw(m_titleText);
    
    for (const auto& row : m_tableRows) {
        window.draw(row);
    }
    
    window.draw(m_backButton.rect);
    window.draw(m_backButton.text);
}
