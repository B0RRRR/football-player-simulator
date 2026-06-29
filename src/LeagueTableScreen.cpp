#include "UITheme.h"
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
        sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y); sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        
        if (m_backButton.rect.getGlobalBounds().contains(mousePos)) {
            m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
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
    for (const auto& l : m_gameManager->getDatabase().getLeagues()) {
        for (const auto& c : l.clubs) {
            if (c.name == p->currentClub->name) {
                currentLeague = &l;
                break;
            }
        }
    }
    
    if (!currentLeague) return;
    
    std::string titleStr = currentLeague->name + " Standings";
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
    
    float startY = 100.f;
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
    
    window.draw(m_backButton.rect);
    window.draw(m_backButton.text);
}
