#include "SquadScreen.h"
#include "CareerHubScreen.h"
#include "GameManager.h"
#include "CareerManager.h"
#include "Player.h"
#include "AssetManager.h"
#include "SeasonEndScreen.h"
#include <iostream>
#include <algorithm>

SquadScreen::SquadScreen() {
}

void SquadScreen::init() {
    m_font = AssetManager::get().getFont("MainFont");
    
    Player* p = m_gameManager->getPlayer();
    if (!p || !p->currentClub) return;
    
    std::string titleStr = p->currentClub->name + " - Squad";
    m_titleText.setFont(m_font);
    m_titleText.setString(sf::String::fromUtf8(titleStr.begin(), titleStr.end()));
    m_titleText.setCharacterSize(30);
    m_titleText.setFillColor(sf::Color::White);
    m_titleText.setPosition(50.f, 30.f);
    
    // Back button
    m_btnBack.setSize(sf::Vector2f(150.f, 40.f));
    m_btnBack.setPosition(50.f, 520.f);
    m_btnBack.setFillColor(UITheme::ButtonNormal);
    
    m_btnBackText.setFont(m_font);
    m_btnBackText.setString("Back");
    m_btnBackText.setCharacterSize(20);
    m_btnBackText.setFillColor(sf::Color::White);
    sf::FloatRect br = m_btnBackText.getLocalBounds();
    m_btnBackText.setOrigin(br.left + br.width/2.f, br.top + br.height/2.f);
    m_btnBackText.setPosition(m_btnBack.getPosition().x + m_btnBack.getSize().x/2.f,
                              m_btnBack.getPosition().y + m_btnBack.getSize().y/2.f);
                              
    auto createHeader = [&](sf::Text& t, const std::string& str, float x) {
        t.setFont(m_font);
        t.setString(str);
        t.setCharacterSize(20);
        t.setFillColor(sf::Color(200, 200, 200));
        t.setPosition(x, 110.f);
    };
    
    createHeader(m_headerName, "Name", 50.f);
    createHeader(m_headerPos, "Pos", 350.f);
    createHeader(m_headerNat, "Nat", 450.f);
    createHeader(m_headerOvr, "OVR", 550.f);
    createHeader(m_headerGoals, "G", 650.f);
    createHeader(m_headerAssists, "A", 700.f);
    
    m_rows.clear();
    
    Club* club = p->currentClub;
    auto roster = club->roster; // copy of pointers
    
    // Sort: GK -> DEF -> MID -> FWD, then OVR
    std::sort(roster.begin(), roster.end(), [](AIPlayer* a, AIPlayer* b) {
        if (a->position != b->position) return static_cast<int>(a->position) < static_cast<int>(b->position);
        return a->overall > b->overall;
    });
    
    // The player themselves is also in the team! We should add them to the visual list.
    // They are not in roster, but we can prepend them.
    int i = 0;
    
    auto addPlayerRow = [&](const std::string& name, PlayerPosition pos, const std::string& nat, int ovr, int g, int a, bool isMe) {
        PlayerRow r;
        r.bg.setSize(sf::Vector2f(720.f, m_rowHeight - 4.f));
        r.bg.setPosition(50.f, m_startY + i * m_rowHeight);
        r.bg.setFillColor(isMe ? sf::Color(100, 200, 100, 100) : sf::Color(255, 255, 255, 20));
        
        auto setupText = [&](sf::Text& t, const std::string& str, float x) {
            t.setFont(m_font);
            t.setString(sf::String::fromUtf8(str.begin(), str.end()));
            t.setCharacterSize(18);
            t.setFillColor(sf::Color::White);
            t.setPosition(x, m_startY + i * m_rowHeight + 5.f);
        };
        
        setupText(r.name, name, 50.f);
        std::string posStr = "Unknown";
        if (pos == PlayerPosition::Goalkeeper) posStr = "GK";
        else if (pos == PlayerPosition::Defender) posStr = "DEF";
        else if (pos == PlayerPosition::Midfielder) posStr = "MID";
        else if (pos == PlayerPosition::Forward) posStr = "FWD";
        
        setupText(r.pos, posStr, 350.f);
        setupText(r.nat, nat.substr(0, 3), 450.f); // First 3 letters of nation
        setupText(r.ovr, std::to_string(ovr), 550.f);
        setupText(r.goals, std::to_string(g), 650.f);
        setupText(r.assists, std::to_string(a), 700.f);
        
        m_rows.push_back(r);
        i++;
    };
    
    // Add User
    int userOvr = (p->shooting + p->passing + p->tackling + p->goalkeeping) / 4;
    addPlayerRow(p->name + " (You)", p->position, p->nationality, userOvr, p->goals, p->assists, true);
    
    // Add AI Teammates
    for (auto aip : roster) {
        addPlayerRow(aip->name, aip->position, aip->nationality, aip->overall, aip->goals, aip->assists, false);
    }
    
    m_maxScroll = std::max(0.f, (m_rows.size() * m_rowHeight) - 350.f);
}

void SquadScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y);
        sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        
        if (m_btnBack.getGlobalBounds().contains(mousePos)) {
            if (m_gameManager->getPlayer()->weeksPlayed >= m_gameManager->getCareerManager()->getSeasonLength()) {
                m_gameManager->changeScreen(std::make_shared<SeasonEndScreen>());
            } else {
                m_gameManager->changeScreen(std::make_shared<CareerHubScreen>());
            }
        }
    }
    
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2i pixelPos(event.mouseMove.x, event.mouseMove.y);
        sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
        m_btnBackHovered = m_btnBack.getGlobalBounds().contains(mousePos);
    }
    
    if (event.type == sf::Event::MouseWheelScrolled) {
        m_scrollOffset -= event.mouseWheelScroll.delta * 20.f;
        if (m_scrollOffset < 0.f) m_scrollOffset = 0.f;
        if (m_scrollOffset > m_maxScroll) m_scrollOffset = m_maxScroll;
    }
}

void SquadScreen::update(sf::Time deltaTime) {
    m_btnBack.setFillColor(m_btnBackHovered ? UITheme::ButtonHover : UITheme::ButtonNormal);
    
    for (size_t i = 0; i < m_rows.size(); ++i) {
        float y = m_startY + i * m_rowHeight - m_scrollOffset;
        m_rows[i].bg.setPosition(50.f, y);
        m_rows[i].name.setPosition(60.f, y + 5.f); // slightly offset from BG
        m_rows[i].pos.setPosition(350.f, y + 5.f);
        m_rows[i].nat.setPosition(450.f, y + 5.f);
        m_rows[i].ovr.setPosition(550.f, y + 5.f);
        m_rows[i].goals.setPosition(650.f, y + 5.f);
        m_rows[i].assists.setPosition(700.f, y + 5.f);
    }
}

void SquadScreen::draw(sf::RenderWindow& window) {
    UITheme::drawGradientBackground(window);
    window.draw(m_titleText);
    
    window.draw(m_headerName);
    window.draw(m_headerPos);
    window.draw(m_headerNat);
    window.draw(m_headerOvr);
    window.draw(m_headerGoals);
    window.draw(m_headerAssists);
    
    // Scroll area clipping
    sf::View defaultView = window.getView();
    sf::View scrollView = defaultView;
    
    sf::Vector2u winSize = window.getSize();
    float viewTop = 150.f / winSize.y;
    float viewHeight = 350.f / winSize.y;
    
    scrollView.setViewport(sf::FloatRect(0.f, viewTop, 1.f, viewHeight));
    scrollView.reset(sf::FloatRect(0.f, 150.f, winSize.x, 350.f));
    
    window.setView(scrollView);
    
    for (auto& r : m_rows) {
        if (r.bg.getPosition().y + m_rowHeight >= 150.f && r.bg.getPosition().y <= 500.f) {
            window.draw(r.bg);
            window.draw(r.name);
            window.draw(r.pos);
            window.draw(r.nat);
            window.draw(r.ovr);
            window.draw(r.goals);
            window.draw(r.assists);
        }
    }
    
    window.setView(defaultView);
    
    window.draw(m_btnBack);
    window.draw(m_btnBackText);
}
