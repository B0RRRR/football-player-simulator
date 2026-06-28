#include "SeasonEndScreen.h"
#include "TransferScreen.h"
#include "GameManager.h"
#include "Player.h"
#include "AssetManager.h"
#include "LeagueTableScreen.h"
#include "EuropeanCupScreen.h"

SeasonEndScreen::SeasonEndScreen() {
}

void SeasonEndScreen::init() {
    auto& font = AssetManager::get().getFont("MainFont");
    
    m_titleText.setFont(font);
    m_titleText.setCharacterSize(40);
    m_titleText.setFillColor(sf::Color::Yellow);
    m_titleText.setPosition(100.f, 50.f);
    m_titleText.setString("Season Finished!");
    
    m_statsText.setFont(font);
    m_statsText.setCharacterSize(24);
    m_statsText.setFillColor(sf::Color::White);
    m_statsText.setPosition(100.f, 150.f);
    
    m_infoText.setFont(font);
    m_infoText.setCharacterSize(20);
    m_infoText.setFillColor(sf::Color(200, 200, 200));
    m_infoText.setPosition(100.f, 300.f);
    
    auto createBtn = [&](const std::string& text, float y, const std::string& action, sf::Color color) {
        Button btn;
        btn.rect.setSize(sf::Vector2f(300.f, 50.f));
        btn.rect.setPosition(250.f, y);
        btn.rect.setFillColor(color);
        
        btn.text.setFont(font);
        btn.text.setString(text);
        btn.text.setCharacterSize(20);
        btn.text.setFillColor(sf::Color::White);
        
        sf::FloatRect tr = btn.text.getLocalBounds();
        btn.text.setOrigin(tr.left + tr.width/2.0f, tr.top + tr.height/2.0f);
        btn.text.setPosition(btn.rect.getPosition().x + btn.rect.getSize().x/2.0f,
                             btn.rect.getPosition().y + btn.rect.getSize().y/2.0f);
                             
        btn.action = action;
        m_buttons.push_back(btn);
    };
    
    createBtn("View League Table", 400.f, "LEAGUE", sf::Color(70, 70, 150));
    createBtn("View European Cups", 460.f, "CUPS", sf::Color(150, 100, 50));
    createBtn("Proceed to Pre-Season", 520.f, "NEXT", sf::Color(70, 150, 70));
}

void SeasonEndScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                if (btn.action == "NEXT") {
                    // Start new season
                    m_gameManager->getCareerManager()->endSeason();
                    
                    // Force transfer screen
                    m_gameManager->changeScreen(std::make_shared<TransferScreen>());
                } else if (btn.action == "LEAGUE") {
                    m_gameManager->changeScreen(std::make_shared<LeagueTableScreen>());
                } else if (btn.action == "CUPS") {
                    m_gameManager->changeScreen(std::make_shared<EuropeanCupScreen>());
                }
            }
        }
    }
    
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos(static_cast<float>(event.mouseMove.x), static_cast<float>(event.mouseMove.y));
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                btn.rect.setFillColor(sf::Color(100, 200, 100));
            } else {
                btn.rect.setFillColor(sf::Color(70, 150, 70));
            }
        }
    }
}

void SeasonEndScreen::update(sf::Time deltaTime) {
    Player* p = m_gameManager->getPlayer();
    if (!p) return;
    
    std::string stats = "Player: " + p->name + "\n";
    stats += "Goals: " + std::to_string(p->goals) + "\n";
    stats += "Assists: " + std::to_string(p->assists) + "\n";
    if (p->currentClub) {
        stats += "Club: " + p->currentClub->name + " (" + std::to_string(p->currentClub->points) + " pts)\n";
    }
    
    m_statsText.setString(sf::String::fromUtf8(stats.begin(), stats.end()));
    m_infoText.setString("The season has concluded.\nTeams have been promoted and relegated.\nYou are now 1 year older.\nGet ready for the summer transfer window!");
}

void SeasonEndScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(20, 20, 40));
    window.draw(m_titleText);
    window.draw(m_statsText);
    window.draw(m_infoText);
    
    for (const auto& btn : m_buttons) {
        window.draw(btn.rect);
        window.draw(btn.text);
    }
}
