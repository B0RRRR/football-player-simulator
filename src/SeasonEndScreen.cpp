#include "SeasonEndScreen.h"
#include "TransferScreen.h"
#include "GameManager.h"
#include "Player.h"
#include "AssetManager.h"

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
    
    Button btnNext;
    btnNext.rect.setSize(sf::Vector2f(300.f, 50.f));
    btnNext.rect.setPosition(250.f, 450.f);
    btnNext.rect.setFillColor(sf::Color(70, 150, 70));
    
    btnNext.text.setFont(font);
    btnNext.text.setString("Proceed to Pre-Season");
    btnNext.text.setCharacterSize(20);
    btnNext.text.setFillColor(sf::Color::White);
    
    sf::FloatRect tr = btnNext.text.getLocalBounds();
    btnNext.text.setOrigin(tr.left + tr.width/2.0f, tr.top + tr.height/2.0f);
    btnNext.text.setPosition(btnNext.rect.getPosition().x + btnNext.rect.getSize().x/2.0f,
                             btnNext.rect.getPosition().y + btnNext.rect.getSize().y/2.0f);
                             
    btnNext.action = "NEXT";
    m_buttons.push_back(btnNext);
}

void SeasonEndScreen::handleInput(sf::RenderWindow& window, const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
        for (auto& btn : m_buttons) {
            if (btn.rect.getGlobalBounds().contains(mousePos)) {
                if (btn.action == "NEXT") {
                    // Start new season
                    m_gameManager->getCareerManager()->endSeason();
                    
                    // Force transfer screen
                    m_gameManager->changeScreen(std::make_shared<TransferScreen>());
                }
            }
        }
    }
    
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos(event.mouseMove.x, event.mouseMove.y);
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
